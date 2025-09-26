#pragma once

#include "AnimationManager.h"
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

  ENDPOINT("POST", "/set/{val}", setValue, PATH(Int32, val)) {
    worker->setValue(val);
    return createResponse(Status::CODE_200, "Value set");
  }

  ENDPOINT("GET", "/get", getValue) {
    int val = worker->getValue();
    return createResponse(Status::CODE_200, std::to_string(val));
  }
};

#include OATPP_CODEGEN_END(ApiController) ///< End ApiController codegen section
