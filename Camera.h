#ifndef CAMERA_CLASS_H
#define CAMERA_CLASS_H 

#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<glm/gtx/rotate_vector.hpp>
#include<glm/gtx//vector_angle.hpp>

#include"shaderClass.h"

class Camera
{
	public:
		glm::vec3 Position;
		glm::vec3 Orientation = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::mat4 cameraMatrix = glm::mat4(1.0f);
		glm::mat4 projectionMatrix;

		bool firstClick = true;

		int width;
		int height;

		float speed = 0.0001f;
		float sensitivity = 50.0f;

		bool UIMode = false;
		bool editorCameraEnabled = false;

		Camera(int width, int height, glm::vec3 position);
		
		glm::mat4 GetViewMatrix() const { return glm::lookAt(Position, Position + Orientation, Up); }
		// Updates and exports the camera matrix to the Vertex Shader
		void updateMatrix(float FOVdeg, float nearPlane, float farPlane);
		void Matrix(Shader& shader, const char* uniform);
		// Handles camera inputs
		void Inputs(GLFWwindow* window);
};
#endif