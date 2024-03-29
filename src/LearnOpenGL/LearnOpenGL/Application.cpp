#include "Application.h"

void init()
{
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);

	loadModels();
	loadTextures();

	// instancing
	const int nrRows = 5;
	const int nrCols = 5;

	glm::vec2 offsets[nrRows * nrCols * 4];

	unsigned int index = 0;

	for (int y = -nrRows + 1; y < nrRows; ++y) {
		for (int x = -nrCols + 1; x < nrCols; ++x)
		{
			glm::vec2 offset;
			offset.x = float(x) / float(nrCols);
			offset.y = float(y) / float(nrRows);
			offsets[index++] = offset;

			cout << "[" << offsets[index - 1].x << ", " << offsets[index - 1].y << "] ";
		}
		cout << "index = " << index << endl;
	}
	
	instancingShader = std::make_unique<Shader>("Shaders/instancing.vs", "Shaders/instancing.fs");

	instancingShader->use();
	for (unsigned int i = 0; i < nrRows * nrCols * 4; ++i)
		instancingShader->setVec2("offsets[" + to_string(i) + "]", offsets[i]);

	// UBO
	glGenBuffers(1, &uboMatrices);
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboMatrices);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void loop()
{	
	// ----------------------------------------
	// render
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 projection = glm::perspective(glm::radians(camera.Fov), SCR_WIDTH / float(SCR_HEIGHT), 0.1f, 100.f);
	glm::mat4 view = camera.GetViewMatrix();

	// ---- Scene
	// UBO
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &projection);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	instancingShader->use();

	glm::mat4 model = glm::scale(glm::mat4(1.0), glm::vec3(0.05f));
	model = glm::rotate(model, float(glfwGetTime()), glm::vec3(0, 0, 1));
	instancingShader->setMat4("model", model);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, containerTexture);

	glBindVertexArray(screenVAO);
	//glDrawArrays(GL_TRIANGLES, 0, 6);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 100);

	glBindVertexArray(0);
}

int main()
{
	if (initWindow() != 0)
		return -1;

	init();

	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = float(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		loop();

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glBindVertexArray(0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glDeleteBuffers(1, &cubeVBO);
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteBuffers(1, &planeVBO);
	glDeleteVertexArrays(1, &planeVAO);
	glDeleteBuffers(1, &cubemapVBO);
	glDeleteVertexArrays(1, &cubemapVAO);
	glDeleteBuffers(1, &reflectiveCubeVBO);
	glDeleteVertexArrays(1, &reflectiveCubeVAO);

	glfwTerminate();
	return 0;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camera.ProcessKeyboard(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera.ProcessKeyboard(DOWN, deltaTime);

	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
	{
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
}

void mouse_callback(GLFWwindow* window, const double xpos, const double ypos)
{
	if (firstMouse) {
		xLast = float(xpos);
		yLast = float(ypos);
		firstMouse = false;
	}

	const float xOffset = float(xpos) - xLast;
	const float yOffset = yLast - float(ypos); // reversed since y-coordinates go from bottom to top
	xLast = float(xpos);
	yLast = float(ypos);

	camera.ProcessMouseMovement(xOffset, yOffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(float(yoffset));
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}


// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front) 
// -Z (back)
// -------------------------------------------------------
unsigned int loadCubemapTexture(const std::vector<string>& faces, const string &directory)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); ++i)
	{
		string path = directory + faces[i];
		unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}