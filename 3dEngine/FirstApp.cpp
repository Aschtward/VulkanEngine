// project includes
#include "first_app.hpp"
#include "simple_renderer_system.hpp"
#include "lve_camera.hpp"
#include "keyboard_movement_controller.hpp"

//glm includes
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm/glm.hpp>
#include <glm/gtc/constants.hpp>

//c includes
#include <stdexcept>
#include <array>
#include <chrono>

namespace lve{

	FirstApp::FirstApp() {
		loadGameoObjects();
		
	}

	FirstApp::~FirstApp() {}

	void FirstApp::run() {

		SimpleRenderSystem simpleRenderSystem{lveDevice, lveRenderer.getSwapChainRenderPass()};

        Camera camera{};

        camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f,0.f,2.5f));

        auto viewerObject = LveGameObject::createGameObject();
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
            camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10.f);

			if (auto commandBuffer = lveRenderer.beginFrame()) {
				lveRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
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
        cube.transform.translation = { .0f, .5f, 2.5f };
        cube.transform.scale = { .5f, .5f, .5f };
        gameObjects.push_back(std::move(cube));

	}

	
}