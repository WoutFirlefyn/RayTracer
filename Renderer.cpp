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
#include <execution>

#define PARALLEL_EXECUTION

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
	const Matrix& cameraToWorld{ camera.CalculateCameraToWorld() };
	//const auto& materials = pScene->GetMaterials();
	//const auto& lights = pScene->GetLights();

	const float aspectRatio{ float(m_Width) / m_Height };
	Ray viewRay{ camera.origin};

	float fov{ tan(camera.fovAngle * TO_RADIANS / 2.f)  };

#if defined(PARALLEL_EXECUTION)
	// Parallel logic
	const uint32_t amountOfPixels{ uint32_t(m_Width * m_Height) };
	std::vector<uint32_t> pixelIndices{};

	pixelIndices.reserve(amountOfPixels);
	for (uint32_t index{}; index < amountOfPixels; ++index) pixelIndices.emplace_back(index);

	std::for_each(std::execution::par, pixelIndices.begin(), pixelIndices.end(), [&](int i) {
		RenderPixel(pScene, i, fov, aspectRatio, cameraToWorld, camera.origin);
		});
#else
	// Synchronous logic (no threading)
	uint32_t amountOfPixels{ uint32_t(m_Width * m_Height) };
	for (uint32_t pixelIndex{}; pixelIndex < amountOfPixels; pixelIndex++)
		RenderPixel(pScene, pixelIndex, fov, aspectRatio, cameraToWorld, camera.origin);
#endif
	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::RenderPixel(Scene* pScene, uint32_t pixelIndex, float fov, float aspectRatio, const Matrix& cameraToWorld, const Vector3& cameraOrigin)const
{
	const auto& materials = pScene->GetMaterials();

	const uint32_t px{ pixelIndex % m_Width }, py{ pixelIndex / m_Width };
	const float cx{ (2 * ((px + 0.5f) / m_Width) - 1) * aspectRatio * fov };
	const float cy{ (1 - (2 * ((py + 0.5f) / m_Height))) * fov };

	ColorRGB finalColor{};

	Vector3 rayDirection{ cx, cy, 1 };
	rayDirection = cameraToWorld.TransformVector(rayDirection).Normalized();
	const Ray& viewRay{ cameraOrigin, rayDirection };

	HitRecord closestHit{};

	pScene->GetClosestHit(viewRay, closestHit);

	if (closestHit.didHit)
	{
		Ray lightRay{ closestHit.origin, {}, 0.01f, };

		for (const Light& light : pScene->GetLights())
		{
			Vector3 directionToLight{ LightUtils::GetDirectionToLight(light, closestHit.origin) };
			lightRay.max = directionToLight.Normalize();
			lightRay.direction = directionToLight;

			bool renderShadow{ m_ShadowsEnabled };
			if (m_ShadowsEnabled)
				renderShadow = pScene->DoesHit(lightRay);

			if (renderShadow)
				continue;

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
	//Update Color in Buffer
	finalColor.MaxToOne();

	m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBuffer->format,
		static_cast<uint8_t>(finalColor.r * 255),
		static_cast<uint8_t>(finalColor.g * 255),
		static_cast<uint8_t>(finalColor.b * 255));
}

float Renderer::CalculateObservedArea(const Ray& ray, const Vector3& normal) const
{
	return std::max(0.f, Vector3::Dot(ray.direction, normal) / sqrtf(ray.direction.SqrMagnitude() * normal.SqrMagnitude()));
}

void Renderer::CycleLightingMode()
{
	m_CurrentLightingMode = static_cast<LightingMode>((int(m_CurrentLightingMode) + 1) % 4);
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBuffer, "RayTracing_Buffer.bmp");
}
