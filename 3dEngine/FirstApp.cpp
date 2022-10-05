// project includes
#include "first_app.hpp"
#include "simple_renderer_system.hpp"
#include "lve_camera.hpp"
#include "keyboard_movement_controller.hpp"
#include "lve_buffer.hpp"
#include "point_light_system.hpp"

//glm includes
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

//c includes
#include <stdexcept>
#include <array>
#include <chrono>
#include <numeric>

namespace lve{

	FirstApp::FirstApp() {
		globalPool = LveDescriptorPool::Builder(lveDevice).setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
			.build();

		loadGameoObjects();
	}

	FirstApp::~FirstApp() {}

	void FirstApp::run() {

		std::vector<std::unique_ptr<LveBuffer>> uboBuffer(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < uboBuffer.size(); i++) {
			uboBuffer[i] = std::make_unique<LveBuffer>(
				lveDevice,
				sizeof(GlobalUbo),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT	 
				);
			uboBuffer[i]->map();
		}

		auto globalSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , VK_SHADER_STAGE_ALL_GRAPHICS)
			.build();

		std::vector<VkDescriptorSet> globalDescriptorSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uboBuffer[i]->descriptorInfo();
			LveDescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.build(globalDescriptorSets[i]);
		}

		SimpleRenderSystem simpleRenderSystem{lveDevice, lveRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
		PointLightSystem pointLightSystem{ lveDevice, lveRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
        
		Camera camera{};

        camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f,0.f,2.5f));

        auto viewerObject = LveGameObject::createGameObject();
		viewerObject.transform.translation.z = -2.5f;

        KeyboardMovementController cameraController{};

        auto currentTime = std::chrono::high_resolution_clock::now();

		while (!lveWindow.shouldClose()) {
			glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            cameraController.moveInPlaneXZ(lveWindow.getGLFWwindow(), frameTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

            float aspect = lveRenderer.getAspectRatio();
            camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);

			if (auto commandBuffer = lveRenderer.beginFrame()) {
				int frameIndex = lveRenderer.getFrameIndex();
				FrameInfo frameInfo{
					frameIndex,
					frameTime,
					commandBuffer,
					camera,
					globalDescriptorSets[frameIndex],
					gameObjects
				};

				//update

				GlobalUbo ubo{};
				ubo.projection = camera.getProjection();
				ubo.view = camera.getViewMatrix();
				ubo.inverseView = camera.getinverseViewMatrix();
				pointLightSystem.update(frameInfo, ubo);
				uboBuffer[frameIndex]->writeToBuffer(&ubo);
				uboBuffer[frameIndex]->flush();

				//render

				lveRenderer.beginSwapChainRenderPass(commandBuffer);

				simpleRenderSystem.renderGameObjects(frameInfo);
				pointLightSystem.render(frameInfo);

				lveRenderer.endSwapChainRenderPass(commandBuffer);
				lveRenderer.endFrame();
			}
		}
		vkDeviceWaitIdle(lveDevice.device());
	}

	void FirstApp::loadGameoObjects() {
        std::shared_ptr<LveModel> lveModel = LveModel::createModelFromFile(lveDevice,"D:\\Pos\\3dEngine\\3dEngine\\models\\smooth_vase.obj");
        auto cube = LveGameObject::createGameObject();
        cube.model = lveModel;
        cube.transform.translation = { .0f, .5f, .0f };
        cube.transform.scale = { 2.5f, 2.5f, 2.5f };
        gameObjects.emplace(cube.getId(),std::move(cube));

		lveModel = LveModel::createModelFromFile(lveDevice, "D:\\Pos\\3dEngine\\3dEngine\\models\\quad.obj");
		auto quad = LveGameObject::createGameObject();
		quad.model = lveModel;
		quad.transform.translation = { .0f, .5f, .0f };
		quad.transform.scale = { 2.3f, 2.1f, 2.3f };
		gameObjects.emplace(quad.getId(), std::move(quad));

		std::vector<glm::vec3> lightColors{
		 {1.f, .1f, .1f},
		 {.1f, .1f, 1.f},
		 {.1f, 1.f, .1f},
		 {1.f, 1.f, .1f},
		 {.1f, 1.f, 1.f},
		 {1.f, 1.f, 1.f}  
		};

		for (int i = 0; i < lightColors.size(); i++) {
			auto pointLight = LveGameObject::makePointLight(0.2f);
			pointLight.color = lightColors[i];
			auto rotateLight = glm::rotate(
				glm::mat4(1.f),
				i * glm::two_pi<float>() / lightColors.size(),
				{ 0.f,-1.f,0.f }
			);
			pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
			gameObjects.emplace(pointLight.getId(), std::move(pointLight));
		}

	}

	
}