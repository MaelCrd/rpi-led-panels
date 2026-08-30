#pragma once

#include "AnimationManager.h"
#include "FastNoise/Base64.h"
#include "oatpp/macro/codegen.hpp"
#include "oatpp/macro/component.hpp"
#include "oatpp/web/server/api/ApiController.hpp"

#include <memory>

#include OATPP_CODEGEN_BEGIN(ApiController) ///< Begin ApiController codegen section

class MainController : public oatpp::web::server::api::ApiController {
private:
  std::shared_ptr<AnimationManager> worker;

public:
  MainController(
      OATPP_COMPONENT(std::shared_ptr<oatpp::web::mime::ContentMappers>,
                      apiContentMappers),
      std::shared_ptr<AnimationManager> w = nullptr)
      : oatpp::web::server::api::ApiController(apiContentMappers), worker(w) {}

  ADD_CORS(getCurrentAnimation)
  ADD_CORS(setCurrentAnimation)
  ADD_CORS(getBrightness)
  ADD_CORS(setBrightness)
  ADD_CORS(getAllAnimations)
  ADD_CORS(setAnimationModeInt)
  ADD_CORS(getAnimationMode)
  ADD_CORS(setAnimationParameters)
  ADD_CORS(getState)
  ADD_CORS(setState)

  ENDPOINT("GET", "/animation", getCurrentAnimation) {
    int val = worker->getCurrentAnimation();
    return createResponse(Status::CODE_200, std::to_string(val));
  }

  ENDPOINT("POST", "/animation/{val}", setCurrentAnimation, PATH(Int32, val)) {
    worker->setCurrentAnimation(val);
    return createResponse(Status::CODE_200, "Value set");
  }

  ENDPOINT("GET", "/brightness", getBrightness) {
    int val = worker->getBrightness();
    return createResponse(Status::CODE_200, std::to_string(val));
  }

  ENDPOINT("POST", "/brightness/{val}", setBrightness, PATH(Int32, val)) {
    worker->setBrightness(val);
    return createResponse(Status::CODE_200, "Value set");
  }

  ENDPOINT("GET", "/animations", getAllAnimations) {
    auto json = worker->getAllAnimations();
    return createResponse(Status::CODE_200, json.dump());
  }

  ENDPOINT("POST", "/animation/{animId}/mode/{mode}", setAnimationModeInt,
           PATH(Int32, animId), PATH(Int32, mode)) {
    bool success = worker->setAnimationMode(animId, mode);
    if (success) {
      // Also set this animation as the current one
      worker->setCurrentAnimation(animId);
      return createResponse(Status::CODE_200,
                            "Mode set successfully and animation activated");
    } else {
      return createResponse(Status::CODE_400, "Invalid animation ID or mode");
    }
  }

  ENDPOINT("GET", "/animation/{animId}/mode", getAnimationMode,
           PATH(Int32, animId)) {
    int mode = worker->getAnimationModeInt(animId);
    if (mode == -1) {
      return createResponse(Status::CODE_400, "Invalid animation ID");
    }
    return createResponse(Status::CODE_200, std::to_string(mode));
  }

  ENDPOINT("POST", "/animation/{animId}/parameters", setAnimationParameters,
           PATH(Int32, animId), BODY_STRING(String, body)) {
    if (body == nullptr || body->empty()) {
      return createResponse(Status::CODE_400, "Request body is empty");
    }
    try {
      auto json = nlohmann::json::parse(body->c_str());
      bool success = worker->setAnimationParameters(animId, json);
      if (success) {
        return createResponse(Status::CODE_200, "Parameters set successfully");
      } else {
        return createResponse(Status::CODE_400,
                              "Invalid animation ID or parameters");
      }
    } catch (const std::exception &e) {
      return createResponse(Status::CODE_400, "Invalid JSON format");
    }
  }

  ENDPOINT("GET", "/state", getState) {
    bool val = worker->getState();
    return createResponse(Status::CODE_200, val ? "1" : "0");
  }

  ENDPOINT("POST", "/state/{val}", setState, PATH(Int32, val)) {
    if (val == 1) {
      worker->setState(true);
      return createResponse(Status::CODE_200, "Animation manager turned ON");
    } else if (val == 0) {
      worker->setState(false);
      return createResponse(Status::CODE_200, "Animation manager turned OFF");
    } else {
      return createResponse(Status::CODE_400,
                            "Invalid state value. Use '1'/'0'");
    }
  }
};

#include OATPP_CODEGEN_END(ApiController) ///< End ApiController codegen section
