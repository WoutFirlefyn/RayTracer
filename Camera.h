#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{
		}


		Vector3 origin{};
		float fovAngle{90.f};
		const float minFov{ 20.f };
		const float maxFov{ 120.f };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{0.f};
		float totalYaw{0.f};

		const float movementSpeed{ 5.f };
		const float rotationSpeed{ 1.f };
		const float fovSpeed{ 15.f };

		Matrix cameraToWorld{};


		Matrix CalculateCameraToWorld()
		{
			right = Vector3::Cross(Vector3::UnitY,forward).Normalized();
			up = Vector3::Cross(forward, right).Normalized();

			Matrix newMatrix{ right, up, forward, origin };
			return newMatrix;
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			//todo: W2
			float moveSpeed{ movementSpeed * deltaTime };
			const float rotSpeed{ rotationSpeed * deltaTime };
			const float speedMultiplier{ 4.f };

			if (pKeyboardState[SDL_SCANCODE_LSHIFT])
			{
				moveSpeed *= speedMultiplier;
			}
			if (pKeyboardState[SDL_SCANCODE_W])
			{
				origin += forward * moveSpeed;
			}
			if (pKeyboardState[SDL_SCANCODE_S])
			{
				origin -= forward * moveSpeed;
			}
			if (pKeyboardState[SDL_SCANCODE_D])
			{
				origin += right * moveSpeed;
			}
			if (pKeyboardState[SDL_SCANCODE_A])
			{
				origin -= right * moveSpeed;
			}

			if (pKeyboardState[SDL_SCANCODE_LEFT] && fovAngle > minFov)
			{
				fovAngle -= fovSpeed * deltaTime;
			}
			if (pKeyboardState[SDL_SCANCODE_RIGHT] && fovAngle < maxFov)
			{
				fovAngle += fovSpeed * deltaTime;
			}

			float pitch{}; 
			float yaw{};
			if (mouseState == SDL_BUTTON(1))
			{
				yaw = mouseX * rotSpeed;
				origin += (float)std::max(-5, std::min(5, -mouseY)) * forward * moveSpeed;
			}
			if (mouseState == SDL_BUTTON(3))
			{
				yaw = mouseX * rotSpeed;
				pitch = -mouseY * rotSpeed;
			}
			
			totalPitch += pitch;
			if (totalPitch >= PI_DIV_2) totalPitch = PI_DIV_2 - 0.01f;
			if (totalPitch <= -PI_DIV_2) totalPitch = -PI_DIV_2 + 0.01f;
			totalYaw += yaw;

			Matrix rotationMatrix{ Matrix::CreateRotationX(totalPitch) * Matrix::CreateRotationY(totalYaw) };
			forward = rotationMatrix.TransformVector(Vector3::UnitZ);
			forward.Normalize();
		}
	};
}
