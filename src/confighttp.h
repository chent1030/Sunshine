/**
 * @file src/confighttp.h
 * @brief Declarations for the loopback-only JSON configuration API server.
 */
#pragma once

// standard includes
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

// lib includes
#include <libvirtualhid/license.hpp>
#include <nlohmann/json.hpp>
#include <Simple-Web-Server/server_https.hpp>

// local includes
#include "thread_safe.h"

namespace confighttp {
  constexpr auto PORT_HTTPS = 1;  ///< GameStream port offset for port https.

  // Type aliases for HTTPS server components
  using https_server_t = SimpleWeb::Server<SimpleWeb::HTTPS>;
  using resp_https_t = std::shared_ptr<typename SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Response>;
  using req_https_t = std::shared_ptr<typename SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Request>;

  // Main server start function
  void start();

  void print_req(const req_https_t &request);
  void send_response(const resp_https_t &response, const nlohmann::json &output_tree);
  void send_unauthorized(const resp_https_t &response, const req_https_t &request);
  bool authenticate(const resp_https_t &response, const req_https_t &request);
  void not_found(const resp_https_t &response, const std::string &error_message = "Not Found");
  void bad_request(const resp_https_t &response, const std::string &error_message = "Bad Request");
  /**
   * @brief Check content type.
   *
   * @param response HTTP response object to populate.
   * @param request HTTP request data from the client.
   * @param contentType Expected HTTP content type.
   * @return True when the request passes validation and processing may continue.
   */
  bool check_content_type(const resp_https_t &response, const req_https_t &request, const std::string_view &contentType);
  std::string generate_csrf_token(const std::string &client_id);
  /**
   * @brief Validate CSRF token.
   *
   * @param response HTTP response object to populate.
   * @param request HTTP request data from the client.
   * @param client_id Client identifier used to look up the CSRF token.
   * @return True when the request passes validation and processing may continue.
   */
  bool validate_csrf_token(const resp_https_t &response, const req_https_t &request, const std::string &client_id);
  std::string get_client_id(const req_https_t &request);
  /**
   * @brief Validate an application index and report an invalid index through the response.
   *
   * @param response HTTP response object to populate.
   * @param index Zero-based index of the item being addressed.
   * @return True when the index identifies an existing application.
   */
  bool check_app_index(const resp_https_t &response, int index);
  void getLocale(const resp_https_t &response, const req_https_t &request);

  /**
   * @brief List authenticated pairing requests awaiting operator approval.
   *
   * @param response HTTP response object to populate.
   * @param request HTTP request data from the client.
   */
  void getPendingPairings(const resp_https_t &response, const req_https_t &request);

  /**
   * @brief Cancel an explicitly selected pairing request.
   *
   * @param response HTTP response object to populate.
   * @param request HTTP request data from the client.
   */
  void cancelPairing(const resp_https_t &response, const req_https_t &request);

  /**
   * @brief Apply a PIN to an explicitly selected pairing request.
   *
   * @param response HTTP response object to populate.
   * @param request HTTP request data from the client.
   */
  void savePin(const resp_https_t &response, const req_https_t &request);

  /**
   * @brief Check whether a detected driver version identifies a development build.
   *
   * Numeric versions beginning with `0.0` and containing at least three
   * components are development builds.
   *
   * @param version Detected driver version.
   * @return True when the version identifies a development build.
   */
  bool is_driver_version_development(std::string_view version);

  /**
   * @brief Check whether a detected driver version satisfies a minimum version.
   *
   * Empty minimum versions accept any detected version. Non-empty minimum versions
   * require a fully numeric dotted version string. Development versions beginning
   * with `0.0` are always accepted.
   *
   * @param version Detected driver version.
   * @param minimum_version Minimum supported driver version, or empty for any version.
   * @return True when the driver version is supported.
   */
  bool is_driver_version_supported(std::string_view version, std::string_view minimum_version);

  /**
   * @brief Build a standard driver status response.
   *
   * @param installed Whether the driver was detected.
   * @param version Detected driver version.
   * @param minimum_version Minimum supported driver version, or empty for any version.
   * @return Driver status JSON object.
   */
  nlohmann::json build_driver_status(bool installed, const std::string &version, std::string_view minimum_version);

  /**
   * @brief Convert a libvirtualhid license result into an API response.
   *
   * @param result License operation result.
   * @return License status JSON object without the submitted license key.
   */
  nlohmann::json build_virtualhid_license_status(const lvh::LicenseResult &result);

  /**
   * @brief Build Virtual HID Broker version and installation status.
   *
   * @return Virtual HID Broker status JSON.
   */
  nlohmann::json get_virtualhid_driver_status();

  /**
   * @brief Build ViGEmBus fallback driver version and installation status.
   *
   * @return ViGEmBus fallback driver status JSON.
   */
  nlohmann::json get_vigembus_driver_status();

  void getVirtualInputStatus(const resp_https_t &response, const req_https_t &request);

  /**
   * @brief Return the host's permission statuses to the API client.
   *
   * @param response HTTPS response receiving the status JSON.
   * @param request Authenticated HTTPS request.
   */
  void getPermissions(const resp_https_t &response, const req_https_t &request);

  /**
   * @brief Initiate a native permission request from the API client.
   *
   * @param response HTTPS response receiving the request result.
   * @param request Authenticated, CSRF protected request.
   */
  void requestPermission(const resp_https_t &response, const req_https_t &request);

  void getVirtualInputLicense(const resp_https_t &response, const req_https_t &request);

  void updateVirtualInputLicense(const resp_https_t &response, const req_https_t &request);

  void resetPortalToken(const resp_https_t &response, const req_https_t &request);

  /**
   * @brief Save an application with optional fields normalized before persistence.
   */
  void saveApp(const resp_https_t &response, const req_https_t &request);

  /**
   * @brief Update stored credentials, or create them on first run without authentication.
   *
   * @param response HTTPS response receiving the operation status.
   * @param request HTTPS request carrying the credential payload.
   */
  void savePassword(const resp_https_t &response, const req_https_t &request);

#ifdef SUNSHINE_TESTS
  /**
   * @brief Replace native permission statuses for deterministic HTTP tests.
   *
   * @param permissions JSON array returned by the status endpoint.
   */
  void set_permission_statuses_for_testing(nlohmann::json permissions);

  /**
   * @brief Restore native permission status queries after HTTP tests.
   */
  void reset_permission_statuses_for_testing();

  using virtual_input_license_status_provider_t = std::function<lvh::LicenseResult()>;  ///< Test provider for current libvirtualhid license status.
  using portal_token_path_provider_t = std::function<std::filesystem::path()>;  ///< Test provider for the XDG Portal token path.

  /**
   * @brief Replace the virtual-input license status provider for unit tests.
   *
   * @param status_provider Provider returning the license status for the response.
   */
  void set_virtual_input_license_status_provider_for_testing(virtual_input_license_status_provider_t status_provider);

  /**
   * @brief Restore the production virtual-input license status provider after a unit test.
   */
  void reset_virtual_input_license_status_provider_for_testing();

  /**
   * @brief Replace the XDG Portal token path provider for unit tests.
   *
   * @param path_provider Provider returning the token path used by the reset endpoint.
   */
  void set_portal_token_path_provider_for_testing(portal_token_path_provider_t path_provider);

  /**
   * @brief Restore the production XDG Portal token path provider after a unit test.
   */
  void reset_portal_token_path_provider_for_testing();

  /**
   * @brief Exercise request-local sensitive string clearing for unit tests.
   *
   * @param value Mutable sensitive string to overwrite and clear.
   */
  void clear_sensitive_string_for_testing(std::string &value);
#endif
}  // namespace confighttp
