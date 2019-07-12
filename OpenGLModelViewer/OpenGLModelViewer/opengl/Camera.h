#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float ROLL = 1.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
	// Camera Attributes
    glm::vec3 Position;
	glm::vec3 DefaultPosition;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
	glm::vec3 SliderRotation;
	glm::vec3 SliderLastRotation;
	glm::vec3 SliderTransform;
	glm::vec3 SliderLastTransform;
    // Euler Angles
    float Yaw;
    float Pitch;
	float Roll;
    // Camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // Constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f)) : 
		Front(glm::vec3(0.0f, 0.0f, -1.0f)),
		WorldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
		SliderRotation(glm::vec3(0.0f, 0.0f, 0.0f)),
		SliderLastRotation(glm::vec3(0.0f, 0.0f, 0.0f)),
		SliderTransform(glm::vec3(0.0f, 0.0f, 0.0f)),
		SliderLastTransform(glm::vec3(0.0f, 0.0f, 0.0f)),
		MovementSpeed(SPEED), 
		MouseSensitivity(SENSITIVITY), 
		Zoom(ZOOM)
    {
        Position = position;
		DefaultPosition = position;
        Yaw = YAW;
        Pitch = PITCH;
		Roll = ROLL;
        updateCameraVectors();
    }

	void Reset()
	{
		SliderRotation = glm::vec3(0.0f, 0.0f, 0.0f);
		SliderLastRotation = glm::vec3(0.0f, 0.0f, 0.0f);
		SliderTransform = glm::vec3(0.0f, 0.0f, 0.0f);
		SliderLastTransform = glm::vec3(0.0f, 0.0f, 0.0f);
		Position = DefaultPosition;
		Yaw = YAW;
		Pitch = PITCH;
		Roll = ROLL;
		updateCameraVectors();
	}

    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix()
    {
		// Check for vector changes indicating a need to update
		if (
			SliderTransform.x != SliderLastTransform.x ||
			SliderTransform.y != SliderLastTransform.y ||
			SliderTransform.z != SliderLastTransform.z
			)
		{
			SliderLastTransform.x = SliderTransform.x;
			SliderLastTransform.y = SliderTransform.y;
			SliderLastTransform.z = SliderTransform.z;

			float xOffset = SliderTransform.x * -1;
			float yOffset = SliderTransform.y * -1;
			float zOffset = SliderTransform.z * -1;

			Yaw = YAW + xOffset;
			Pitch = PITCH + yOffset;
			Roll = ROLL + zOffset;

			updateCameraVectors();
		}

        return glm::lookAt(Position, Position + Front, Up);
    }

    // Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
    }

    // Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        // Make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // Update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

	void ProcessManualTransform(float xoffset, float yoffset, float zoffset)
	{
		float xOffset = SliderTransform.x * -1;
		float yOffset = SliderTransform.y * -1;
		float zOffset = SliderTransform.z * -1;

		Yaw = YAW + xOffset;
		Pitch = PITCH + yOffset;
		Roll = ROLL + zOffset;

		updateCameraVectors();
	}

    // Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        if (Zoom >= -45.0f && Zoom <= 45.0f)
            Zoom -= yoffset;
        if (Zoom <= -45.0f)
            Zoom = -45.0f;
        if (Zoom >= 45.0f)
            Zoom = 45.0f;
    }

private:
    // Calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // Calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch * Roll));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        // Also re-calculate the Right and Up vector
        Right = glm::normalize(glm::cross(Front, WorldUp)); // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        Up = glm::normalize(glm::cross(Right, Front));
    }
};
#endif