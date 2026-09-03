#include <atomic>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <ctime>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>
#include <unistd.h>

#include "graphics.h"
#include "led-matrix.h"
#include "oatpp/network/Server.hpp"
#include "oatpp/web/server/HttpRouter.hpp"

#include "AnimationManager.h"
#include "AppComponent.hpp"
#include "controller/MainController.hpp"

using rgb_matrix::RGBMatrix;

std::atomic<int> interrupt_received(0);

static void InterruptHandler(int /*signo*/) { interrupt_received = 1; }

void runServer(std::shared_ptr<AnimationManager> animManager) {
  /* Register Components in scope of run() method */
  AppComponent components;

  /* Get router component */
  OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);

  /* Get content mappers component */
  OATPP_COMPONENT(std::shared_ptr<oatpp::web::mime::ContentMappers>,
                  apiContentMappers);

  /* Create MyController and add all of its endpoints to router */
  router->addController(
      std::make_shared<MainController>(apiContentMappers, animManager));

  /* Get connection handler component */
  OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>,
                  connectionHandler);

  /* Get connection provider component */
  OATPP_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>,
                  connectionProvider);

  /* Create server which takes provided TCP connections and passes them to HTTP
   * connection handler */
  oatpp::network::Server server(connectionProvider, connectionHandler);

  std::thread oatppThread([&server] {
    /* Run server, let it check a lambda-function if it should continue to run
     * Return true to keep the server up, return false to stop it.
     * Treat this function like a ISR: Don't do anything heavy in it! Just check
     * some flags or at max some very lightweight logic. The performance of your
     * REST-API depends on this function returning as fast as possible! */
    std::function<bool()> condition = []() { return !interrupt_received; };

    server.run(condition);
  });

  /* Print info about server port */
  OATPP_LOGi("RPi LED Panels", "Server running on port {}",
             connectionProvider->getProperty("port").toString());

  /* Run Animation Manager */
  animManager->run(&interrupt_received);

  /* Print info about server stopping */
  OATPP_LOGi("RPi LED Panels", "Server stopping...");

  /* First, stop the ServerConnectionProvider so we don't accept any new
   * connections */
  connectionProvider->stop();

  /* Finally, stop the ConnectionHandler and wait until all running connections
   * are closed */
  connectionHandler->stop();

  /* Check if the thread has already stopped or if we need to wait for the
   * server to stop */
  if (oatppThread.joinable()) {

    /* We need to wait until the thread is done */
    oatppThread.join();
  }
}

int main(int argc, char *argv[]) {
  RGBMatrix::Options defaults;
  rgb_matrix::RuntimeOptions runtime_options;
  defaults.rows = 64;
  defaults.cols = 128;
  defaults.chain_length = 2;
  defaults.parallel = 2;
  runtime_options.drop_privileges = -1; // Don't drop privileges
  // defaults.show_refresh_rate = true;
  runtime_options.gpio_slowdown = 4;
  // // Settings to reduce recording's black bars
  // defaults.limit_refresh_rate_hz = 60;
  // defaults.scan_mode = 1;
  std::unique_ptr<RGBMatrix> matrix(
      RGBMatrix::CreateFromFlags(&argc, &argv, &defaults, &runtime_options));
  if (matrix == NULL)
    return 1;

  // It is always good to set up a signal handler to cleanly exit when we
  // receive a CTRL-C for instance. The DrawOnCanvas() routine is looking
  // for that.
  struct sigaction sa{};
  sa.sa_handler = InterruptHandler;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGTERM, &sa, nullptr);
  sigaction(SIGINT, &sa, nullptr);

  ////

  oatpp::Environment::init();
  auto animManager = std::make_shared<AnimationManager>(matrix.get());
  runServer(animManager);
  oatpp::Environment::destroy();

  // Animation finished. Shut down the RGB matrix.
  matrix->Clear();
  return 0;
}
