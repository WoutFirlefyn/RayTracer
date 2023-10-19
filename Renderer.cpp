//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Material.h"
#include "Scene.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window * pWindow) :
	m_pWindow(pWindow),
	m_pBuffer(SDL_GetWindowSurface(pWindow))
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	m_pBufferPixels = static_cast<uint32_t*>(m_pBuffer->pixels);
}

void Renderer::Render(Scene* pScene) const
{
	Camera& camera = pScene->GetCamera();
	auto& materials = pScene->GetMaterials();
	auto& lights = pScene->GetLights();

	float aspectRatio{ float(m_Width) / m_Height };
	Ray viewRay{ camera.origin };

	float fov{ tan(camera.fovAngle * TO_RADIANS / 2.f)  };

	for (int px{}; px < m_Width; ++px)
	{
		float cx{ ((2 * (px + 0.5f) / m_Width) - 1) * aspectRatio * fov };
		for (int py{}; py < m_Height; ++py)
		{
			float cy{ (1 - 2 * (py + 0.5f) / m_Height) * fov};
			ColorRGB finalColor{};

			Vector3 rayDirection{ cx, cy, 1 };
			rayDirection = camera.CalculateCameraToWorld().TransformVector(rayDirection);
			rayDirection.Normalize();

			viewRay.direction = rayDirection;

			HitRecord closestHit{};

			pScene->GetClosestHit(viewRay, closestHit);

			if (closestHit.didHit)
			{
				Ray lightRay{ closestHit.origin, {}, 0.01f,};

				for (const Light& light : lights)
				{
					Vector3 directionToLight{ LightUtils::GetDirectionToLight(light, closestHit.origin) };
					lightRay.max = directionToLight.Normalize();
					lightRay.direction = directionToLight;

					// inverted lightray to fix shadows with front/backface culling
					if (!pScene->DoesHit(lightRay) || !m_ShadowsEnabled)
					{
						switch (m_CurrentLightingMode)
						{
						case LightingMode::ObservedArea:
							finalColor.r = finalColor.g = finalColor.b += CalculateObservedArea(lightRay, closestHit.normal);
							break;
						case LightingMode::Radiance:
							finalColor += LightUtils::GetRadiance(light, closestHit.origin);
							break;
						case LightingMode::BRDF:
							finalColor += materials[closestHit.materialIndex]->Shade(closestHit, directionToLight, -rayDirection);
							break;
						case LightingMode::Combined:
							finalColor += LightUtils::GetRadiance(light, closestHit.origin)
								* materials[closestHit.materialIndex]->Shade(closestHit, directionToLight, -rayDirection)
								* CalculateObservedArea(lightRay, closestHit.normal);
							break;
						}
					}
				}
			}

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}

	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface(m_pWindow);
}

float Renderer::CalculateObservedArea(const Ray& ray, const Vector3& normal) const
{
	return std::max(0.f, Vector3::Dot(ray.direction, normal) / sqrt(ray.direction.SqrMagnitude() * normal.SqrMagnitude()));
}

void Renderer::CycleLightingMode()
{
	m_CurrentLightingMode = static_cast<LightingMode>((int(m_CurrentLightingMode) + 1) % 4);
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBuffer, "RayTracing_Buffer.bmp");
}
