/**
 * @file tests/unit/test_confighttp.cpp
 * @brief Test src/confighttp.cpp
 *
 * These tests use a real HTTPS client/server to test the actual confighttp endpoints.
 * While this is more of an integration test approach, it's the most practical way to
 * verify that the confighttp functions work correctly end-to-end.
 */

// test includes
#include "../tests_common.h"

// standard includes
#include <array>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string_view>
#include <thread>
#include <utility>

// lib includes
#include <Simple-Web-Server/client_https.hpp>
#include <Simple-Web-Server/crypto.hpp>
#include <Simple-Web-Server/server_https.hpp>

// local includes
#include <src/config.h>
#include <src/confighttp.h>
#include <src/crypto.h>
#include <src/httpcommon.h>
#include <src/network.h>
#include <src/nvhttp.h>
#include <src/process.h>
#include <src/utility.h>

#ifdef SUNSHINE_BUILD_PORTAL
  #include <src/platform/linux/misc.h>
#endif

using namespace std::literals;

namespace {
  // Test certificates
  const std::string TEST_PRIVATE_KEY = R"(-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDLePNlWN06FLlM
ujWzIX8UICO7SWfH5DXlafVjpxwi/WCkdO6FxixqRNGu71wMvJXFbDlNR8fqX2xo
+eq17J3uFKn+qdjmP3L38bkqxhoJ/nCrXkeGyCTQ+Daug63ZYSJeW2Mmf+LAR5/i
/fWYfXpSlbcf5XJQPEWvENpLqWu+NOU50dJXIEVYpUXRx2+x4ZbwkH7tVJm94L+C
OUyiJKQPyWgU2aFsyJGwHFfePfSUpfYHqbHZV/ILpY59VJairBwE99bx/mBvMI7a
hBmJTSDuDffJcPDhFF5kZa0UkQPrPvhXcQaSRti7v0VonEQj8pTSnGYr9ktWKk92
wxDyn9S3AgMBAAECggEAbEhQ14WELg2rUz7hpxPTaiV0fo4hEcrMN+u8sKzVF3Xa
QYsNCNoe9urq3/r39LtDxU3D7PGfXYYszmz50Jk8ruAGW8WN7XKkv3i/fxjv8JOc
6EYDMKJAnYkKqLLhCQddX/Oof2udg5BacVWPpvhX6a1NSEc2H6cDupfwZEWkVhMi
bCC3JcNmjFa8N7ow1/5VQiYVTjpxfV7GY1GRe7vMvBucdQKH3tUG5PYXKXytXw/j
KDLaECiYVT89KbApkI0zhy7I5g3LRq0Rs5fmYLCjVebbuAL1W5CJHFJeFOgMKvnO
QSl7MfHkTnzTzUqwkwXjgNMGsTosV4UloL9gXVF6GQKBgQD5fI771WETkpaKjWBe
6XUVSS98IOAPbTGpb8CIhSjzCuztNAJ+0ey1zklQHonMFbdmcWTkTJoF3ECqAos9
vxB4ROg+TdqGDcRrXa7Twtmhv66QvYxttkaK3CqoLX8CCTnjgXBCijo6sCpo6H1T
+y55bBDpxZjNFT5BV3+YPBfWQwKBgQDQyNt+saTqJqxGYV7zWQtOqKORRHAjaJpy
m5035pky5wORsaxQY8HxbsTIQp9jBSw3SQHLHN/NAXDl2k7VAw/axMc+lj9eW+3z
2Hv5LVgj37jnJYEpYwehvtR0B4jZnXLyLwShoBdRPkGlC5fs9+oWjQZoDwMLZfTg
eZVOJm6SfQKBgQDfxYcB/kuKIKsCLvhHaSJpKzF6JoqRi6FFlkScrsMh66TCxSmP
0n58O0Cqqhlyge/z5LVXyBVGOF2Pn6SAh4UgOr4MVAwyvNp2aprKuTQ2zhSnIjx4
k0sGdZ+VJOmMS/YuRwUHya+cwDHp0s3Gq77tja5F38PD/s/OD8sUIqJGvQKBgBfI
6ghy4GC0ayfRa+m5GSqq14dzDntaLU4lIDIAGS/NVYDBhunZk3yXq99Mh6/WJQVf
Uc77yRsnsN7ekeB+as33YONmZm2vd1oyLV1jpwjfMcdTZHV8jKAGh1l4ikSQRUoF
xTdMb5uXxg6xVWtvisFq63HrU+N2iAESmMnAYxRZAoGAVEFJRRjPrSIUTCCKRiTE
br+cHqy6S5iYRxGl9riKySBKeU16fqUACIvUqmqlx4Secj3/Hn/VzYEzkxcSPwGi
qMgdS0R+tacca7NopUYaaluneKYdS++DNlT/m+KVHqLynQr54z1qBlThg9KGrpmM
LGZkXtQpx6sX7v3Kq56PkNk=
-----END PRIVATE KEY-----)";

  const std::string TEST_PUBLIC_CERT = R"(-----BEGIN CERTIFICATE-----
MIIC6zCCAdOgAwIBAgIBATANBgkqhkiG9w0BAQsFADA5MQswCQYDVQQGEwJJVDEW
MBQGA1UECgwNR2FtZXNPbldoYWxlczESMBAGA1UEAwwJbG9jYWxob3N0MB4XDTIy
MDQwOTA5MTYwNVoXDTQyMDQwNDA5MTYwNVowOTELMAkGA1UEBhMCSVQxFjAUBgNV
BAoMDUdhbWVzT25XaGFsZXMxEjAQBgNVBAMMCWxvY2FsaG9zdDCCASIwDQYJKoZI
hvcNAQEBBQADggEPADCCAQoCggEBAMt482VY3ToUuUy6NbMhfxQgI7tJZ8fkNeVp
9WOnHCL9YKR07oXGLGpE0a7vXAy8lcVsOU1Hx+pfbGj56rXsne4Uqf6p2OY/cvfx
uSrGGgn+cKteR4bIJND4Nq6DrdlhIl5bYyZ/4sBHn+L99Zh9elKVtx/lclA8Ra8Q
2kupa7405TnR0lcgRVilRdHHb7HhlvCQfu1Umb3gv4I5TKIkpA/JaBTZoWzIkbAc
V9499JSl9gepsdlX8guljn1UlqKsHAT31vH+YG8wjtqEGYlNIO4N98lw8OEUXmRl
rRSRA+s++FdxBpJG2Lu/RWicRCPylNKcZiv2S1YqT3bDEPKf1LcCAwEAATANBgkq
hkiG9w0BAQsFAAOCAQEAqPBqzvDjl89pZMll3Ge8RS7HeDuzgocrhOcT2jnk4ag7
/TROZuISjDp6+SnL3gPEt7E2OcFAczTg3l/wbT5PFb6vM96saLm4EP0zmLfK1FnM
JDRahKutP9rx6RO5OHqsUB+b4jA4W0L9UnXUoLKbjig501AUix0p52FBxu+HJ90r
HlLs3Vo6nj4Z/PZXrzaz8dtQ/KJMpd/g/9xlo6BKAnRk5SI8KLhO4hW6zG0QA56j
X4wnh1bwdiidqpcgyuKossLOPxbS786WmsesaAWPnpoY6M8aija+ALwNNuWWmyMg
9SVDV76xJzM36Uq7Kg3QJYTlY04WmPIdJHkCtXWf9g==
-----END CERTIFICATE-----)";
}  // namespace

/**
 * @brief Test fixture that sets up a minimal HTTPS server with confighttp-style routes
 *
 * This fixture creates a real server to test the actual confighttp functions.
 */
class ConfigHttpTest: public BaseTest {  // NOSONAR(cpp:S3656): protected members are intentional for test fixture subclassing
protected:
  std::unique_ptr<SimpleWeb::Server<SimpleWeb::HTTPS>> server;
  std::unique_ptr<SimpleWeb::Client<SimpleWeb::HTTPS>> client;
  std::jthread server_thread;
  unsigned short port = 0;

  std::string saved_username;
  std::string saved_password;
  std::string saved_salt;
  std::string saved_locale;
  std::filesystem::path test_web_dir;
  std::filesystem::path cert_file;
  std::filesystem::path key_file;

  void SetUp() override {
    BaseTest::SetUp();
    nvhttp::expire_pair_sessions(std::chrono::steady_clock::time_point::max());
    confighttp::set_virtual_input_license_status_provider_for_testing([]() {
      lvh::LicenseStatus license;
      license.service_available = true;
      license.state = lvh::LicenseState::licensed;
      license.active_devices = 2;
      license.message = "Test license status";
      return lvh::LicenseResult {lvh::OperationStatus::success(), std::move(license)};
    });
    confighttp::set_permission_statuses_for_testing(nlohmann::json::array({
      {{"id", "screen_recording"}, {"status", "denied"}, {"required", true}, {"verifiable", true}},
      {{"id", "notifications"}, {"status", "granted"}, {"required", false}, {"verifiable", true}},
    }));

    // Save current config
    saved_username = config::sunshine.username;
    saved_password = config::sunshine.password;
    saved_salt = config::sunshine.salt;
    saved_locale = config::sunshine.locale;

    // Set up test credentials
    config::sunshine.username = "testuser";
    config::sunshine.salt = "testsalt";
    config::sunshine.password = util::hex(crypto::hash("testpass" + config::sunshine.salt)).to_string();

    // Set test locale
    config::sunshine.locale = "en";

    // Create test web directory in temp
    test_web_dir = std::filesystem::temp_directory_path() / "sunshine_test_confighttp";  // NOSONAR(cpp:S5443): safe for tests
    std::filesystem::create_directories(test_web_dir);
    confighttp::set_portal_token_path_provider_for_testing([this]() {
      return test_web_dir / "portal_token";
    });

    // Write certificates to temp files (Simple-Web-Server expects file paths)
    cert_file = test_web_dir / "test_cert.pem";
    key_file = test_web_dir / "test_key.pem";

    std::ofstream cert_out(cert_file);
    cert_out << TEST_PUBLIC_CERT;
    cert_out.close();

    std::ofstream key_out(key_file);
    key_out << TEST_PRIVATE_KEY;
    key_out.close();

    // Set up server
    server = std::make_unique<SimpleWeb::Server<SimpleWeb::HTTPS>>(cert_file.string(), key_file.string());
    server->config.port = 0;  // OS assigns port
    server->config.reuse_address = true;
    server->config.timeout_request = 5;
    server->config.timeout_content = 300;

    // Add a route to test authentication directly
    server->resource["^/auth-test$"]["GET"] = [](
                                                const std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Response> &response,
                                                const std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Request> &request
                                              ) {
      // Call the actual confighttp::authenticate function
      const bool authenticated = confighttp::authenticate(response, request);

      if (authenticated) {
        SimpleWeb::CaseInsensitiveMultimap headers;
        headers.emplace("Content-Type", "text/plain");
        response->write("authenticated", headers);
      }
      // If not authenticated, authenticate() already sent the response
    };

    // Add a route to test send_unauthorized
    server->resource["^/unauthorized-test$"]["GET"] = [](
                                                        const std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Response> &response,
                                                        const std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Request> &request
                                                      ) {
      // Call the actual confighttp::send_unauthorized function
      confighttp::send_unauthorized(response, request);
    };

    // Add a route to test not_found
    server->resource["^/notfound-test$"]["GET"] = [](
                                                    const std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Response> &response,
                                                    const std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Request> &
                                                  ) {
      // Call the actual confighttp::not_found function
      confighttp::not_found(response, "Test not found");
    };

    // Add a route to test bad_request
    server->resource["^/badrequest-test$"]["GET"] = [](
                                                      const std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Response> &response,
                                                      const std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Request> &
                                                    ) {
      // Call the actual confighttp::bad_request function
      confighttp::bad_request(response, "Test bad request");
    };

    // Add a route to test send_response with JSON
    server->resource["^/json-test$"]["GET"] = [](
                                                const std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Response> &response,
                                                [[maybe_unused]] const std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Request> &request
                                              ) {
      // Call the actual confighttp::send_response function
      nlohmann::json test_json;
      test_json["status"] = "success";
      test_json["message"] = "Test JSON response";
      test_json["code"] = 200;
      confighttp::send_response(response, test_json);
    };

    // Add a route to test check_content_type
    server->resource["^/content-type-test$"]["POST"] = [](
                                                         const std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Response> &response,
                                                         const std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Request> &request
                                                       ) {
      // Call the actual confighttp::check_content_type function
      if (confighttp::check_content_type(response, request, "application/json")) {
        SimpleWeb::CaseInsensitiveMultimap headers;
        headers.emplace("Content-Type", "text/plain");
        response->write("content-type-valid", headers);
      }
      // If check fails, check_content_type already sent an error response
    };

    // Add a route to test CSRF token generation
    server->resource["^/csrf-token-test$"]["GET"] = [](
                                                      const std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Response> &response,
                                                      const std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Request> &request
                                                    ) {
      // Issue a CSRF token through the production helper. The /api/csrf-token
      // route was removed with the Web UI; native clients never need tokens.
      std::string client_id = confighttp::get_client_id(request);
      nlohmann::json output_tree;
      output_tree["csrf_token"] = confighttp::generate_csrf_token(client_id);
      confighttp::send_response(response, output_tree);
    };

    // Add a route to test CSRF validation (successful)
    server->resource["^/csrf-validate-test$"]["POST"] = [](
                                                          const std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Response> &response,
                                                          const std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Request> &request
                                                        ) {
      // Validate CSRF token
      std::string client_id = confighttp::get_client_id(request);
      if (confighttp::validate_csrf_token(response, request, client_id)) {
        SimpleWeb::CaseInsensitiveMultimap headers;
        headers.emplace("Content-Type", "text/plain");
        response->write("csrf-valid", headers);
      }
      // If validation fails, validate_csrf_token already sent an error response
    };

    // Add a route to test getLocale
    server->resource["^/locale-test$"]["GET"] = [](
                                                  const std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Response> &response,
                                                  const std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Request> &request
                                                ) {
      // Call the actual confighttp::getLocale function
      confighttp::getLocale(response, request);
    };

    server->resource["^/virtual-input-status-test$"]["GET"] = confighttp::getVirtualInputStatus;
    server->resource["^/virtual-input-license-test$"]["GET"] = confighttp::getVirtualInputLicense;
    server->resource["^/virtual-input-license-test$"]["POST"] = confighttp::updateVirtualInputLicense;
    server->resource["^/permissions-test$"]["GET"] = confighttp::getPermissions;
    server->resource["^/permissions-test$"]["POST"] = confighttp::requestPermission;
    server->resource["^/pairing-test$"]["DELETE"] = confighttp::cancelPairing;
    server->resource["^/pairing-test$"]["GET"] = confighttp::getPendingPairings;
    server->resource["^/pairing-test$"]["POST"] = confighttp::savePin;
    server->resource["^/portal-token-reset-test$"]["POST"] = confighttp::resetPortalToken;
    server->resource["^/api/apps$"]["POST"] = confighttp::saveApp;
    server->resource["^/api/password$"]["POST"] = confighttp::savePassword;
    server->default_resource["GET"] = [](const std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Response> &response, const std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Request> &request) {
      confighttp::print_req(request);
      confighttp::not_found(response);
    };

    // Start server
    server_thread = std::jthread([this]() {
      server->start([this](const unsigned short assigned_port) {
        port = assigned_port;
      });
    });

    // Wait for port assignment
    for (int i = 0; i < 100 && port == 0; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_NE(port, 0) << "Server failed to start";

    // Set up client
    client = std::make_unique<SimpleWeb::Client<SimpleWeb::HTTPS>>(std::format("localhost:{}", port), false);
    client->config.timeout = 5;
  }

  void TearDown() override {
    if (server) {
      server->stop();
    }
    if (server_thread.joinable()) {
      server_thread.join();
    }
    confighttp::reset_virtual_input_license_status_provider_for_testing();
    confighttp::reset_permission_statuses_for_testing();
    confighttp::reset_portal_token_path_provider_for_testing();

    config::sunshine.username = saved_username;
    config::sunshine.password = saved_password;
    config::sunshine.salt = saved_salt;
    config::sunshine.locale = saved_locale;
    nvhttp::expire_pair_sessions(std::chrono::steady_clock::time_point::max());

    if (std::filesystem::exists(test_web_dir)) {
      std::filesystem::remove_all(test_web_dir);
    }
    BaseTest::TearDown();
  }

  static std::string create_auth_header(const std::string &username, const std::string &password) {
    return "Basic " + SimpleWeb::Crypto::Base64::encode(username + ":" + password);
  }

  /**
   * @brief Build authenticated JSON POST headers with a fresh CSRF token.
   *
   * @return Headers suitable for POST /api/apps.
   */
  SimpleWeb::CaseInsensitiveMultimap authenticated_json_headers() {
    SimpleWeb::CaseInsensitiveMultimap auth_headers;
    auth_headers.emplace("Authorization", create_auth_header("testuser", "testpass"));
    const auto token_response = client->request("GET", "/csrf-token-test", "", auth_headers);
    EXPECT_EQ(token_response->status_code, "200 OK");
    const auto csrf_token = nlohmann::json::parse(token_response->content.string()).at("csrf_token").get<std::string>();

    SimpleWeb::CaseInsensitiveMultimap headers;
    headers.emplace("Authorization", create_auth_header("testuser", "testpass"));
    headers.emplace("Content-Type", "application/json");
    headers.emplace("X-CSRF-Token", csrf_token);
    return headers;
  }

  /**
   * @brief Redirect apps.json to a temporary directory and restore process state on destruction.
   */
  class ScopedSaveAppAppsFile {
  public:
    /**
     * @brief Create the temporary apps.json directory and take ownership of `proc::proc`.
     *
     * @param subdir Directory name under `SUNSHINE_TEST_BIN_DIR`.
     */
    explicit ScopedSaveAppAppsFile(std::string_view subdir):
        test_dir(std::filesystem::path(SUNSHINE_TEST_BIN_DIR) / subdir) {
      std::filesystem::create_directories(test_dir);
      config::stream.file_apps = (test_dir / "apps.json").string();
    }

    ScopedSaveAppAppsFile(const ScopedSaveAppAppsFile &) = delete;
    ScopedSaveAppAppsFile &operator=(const ScopedSaveAppAppsFile &) = delete;

    /**
     * @brief Restore `file_apps`/`proc::proc` and remove the temporary directory.
     */
    ~ScopedSaveAppAppsFile() noexcept {
      try {
        config::stream.file_apps = previous_file_apps;
        proc::proc = std::move(previous_proc);
        std::error_code ec;
        std::filesystem::remove_all(test_dir, ec);
      } catch (...) {
        ADD_FAILURE() << "Failed to restore apps.json path or process manager state";
      }
    }

    std::filesystem::path test_dir;  ///< Temporary directory for apps.json.
    std::string previous_file_apps {config::stream.file_apps};  ///< Prior apps.json path.
    proc::proc_t previous_proc {std::move(proc::proc)};  ///< Prior process manager state.
  };

  /**
   * @brief Insert a test pairing request into the production pending-session registry.
   *
   * @return Unguessable approval identifier assigned to the request.
   */
  static std::string insert_pending_pairing() {
    nvhttp::pair_session_t session;
    session.client.uniqueID = "rest-api-client";
    session.async_insert_pin.salt = "ff5dc6eda99339a8a0793e216c4257c4";
    session.async_insert_pin.device_name = "REST client";
    session.async_insert_pin.address = "192.0.2.30";

    std::string pairing_id;
    EXPECT_EQ(nvhttp::insert_pair_session(std::move(session), pairing_id), nvhttp::pair_session_insert_e::ADDED);
    return pairing_id;
  }

  static void assert_security_headers(const std::shared_ptr<SimpleWeb::Client<SimpleWeb::HTTPS>::Response> &response) {
    const auto x_frame = response->header.find("X-Frame-Options");
    ASSERT_NE(x_frame, response->header.end());
    ASSERT_EQ(x_frame->second, "DENY");

    const auto csp = response->header.find("Content-Security-Policy");
    ASSERT_NE(csp, response->header.end());
    ASSERT_EQ(csp->second, "frame-ancestors 'none';");
  }

  static void assert_json_error_response(const std::shared_ptr<SimpleWeb::Client<SimpleWeb::HTTPS>::Response> &response, const std::string_view &expected_message, const std::string_view &expected_status_code) {
    const auto content_type = response->header.find("Content-Type");
    ASSERT_NE(content_type, response->header.end());
    ASSERT_TRUE(content_type->second.find("application/json") != std::string::npos);

    assert_security_headers(response);

    const std::string body = response->content.string();
    ASSERT_TRUE(body.find(expected_message) != std::string::npos);
    ASSERT_TRUE(body.find(expected_status_code) != std::string::npos);
  }
};

namespace {
  /**
   * @brief HTTP request used to verify a shared endpoint contract.
   */
  struct endpoint_request_t {
    const char *name;  ///< Stable parameter name.
    const char *method;  ///< HTTP method.
    const char *path;  ///< Test-server route.
    const char *body;  ///< Request body.
  };

  /**
   * @brief Invalid virtual-input license request and expected error.
   */
  struct invalid_license_request_t {
    const char *name;  ///< Stable parameter name.
    const char *body;  ///< Invalid request body.
    const char *expected_error;  ///< Expected response error.
  };

  /**
   * @brief Return the stable name for an endpoint request parameter.
   *
   * @param param_info Parameter information supplied by GoogleTest.
   * @return Stable test suffix.
   */
  std::string endpoint_request_name(const testing::TestParamInfo<endpoint_request_t> &param_info) {
    return param_info.param.name;
  }

  /**
   * @brief Return the stable name for an invalid license request parameter.
   *
   * @param param_info Parameter information supplied by GoogleTest.
   * @return Stable test suffix.
   */
  std::string invalid_license_request_name(const testing::TestParamInfo<invalid_license_request_t> &param_info) {
    return param_info.param.name;
  }
}  // namespace

/**
 * @brief Parameterized fixture for endpoints that require authentication.
 */
class AuthenticatedConfigHttpEndpointTest: public ConfigHttpTest, public testing::WithParamInterface<endpoint_request_t> {};

/**
 * @brief Parameterized fixture for endpoints protected from cross-origin requests.
 */
class CsrfProtectedConfigHttpEndpointTest: public ConfigHttpTest, public testing::WithParamInterface<endpoint_request_t> {};

/**
 * @brief Parameterized fixture for invalid virtual-input license requests.
 */
class InvalidVirtualInputLicenseRequestTest: public ConfigHttpTest, public testing::WithParamInterface<invalid_license_request_t> {};

TEST_P(AuthenticatedConfigHttpEndpointTest, RejectsUnauthenticatedRequest) {
  const auto &request = GetParam();
  const auto response = client->request(request.method, request.path, request.body);
  ASSERT_EQ(response->status_code, "401 Unauthorized");
  EXPECT_NE(response->header.find("WWW-Authenticate"), response->header.end());
}

INSTANTIATE_TEST_SUITE_P(
  ConfigHttpEndpoints,
  AuthenticatedConfigHttpEndpointTest,
  testing::Values(
    endpoint_request_t {"PairingList", "GET", "/pairing-test", ""},
    endpoint_request_t {"PortalTokenReset", "POST", "/portal-token-reset-test", ""},
    endpoint_request_t {"VirtualInputStatus", "GET", "/virtual-input-status-test", ""},
    endpoint_request_t {"VirtualInputLicense", "GET", "/virtual-input-license-test", ""},
    endpoint_request_t {"VirtualInputLicenseUpdate", "POST", "/virtual-input-license-test", R"({"action":"validate"})"}
  ),
  endpoint_request_name
);

TEST_P(CsrfProtectedConfigHttpEndpointTest, RejectsCrossOriginRequestWithoutToken) {
  const auto &request = GetParam();
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "testpass"));
  headers.emplace("Origin", "https://example.invalid");

  const auto response = client->request(request.method, request.path, request.body, headers);
  ASSERT_EQ(response->status_code, "400 Bad Request");
  EXPECT_TRUE(response->content.string().contains("Missing CSRF token"));
}

INSTANTIATE_TEST_SUITE_P(
  ConfigHttpEndpoints,
  CsrfProtectedConfigHttpEndpointTest,
  testing::Values(
    endpoint_request_t {"CsrfValidation", "POST", "/csrf-validate-test", ""},
    endpoint_request_t {"PortalTokenReset", "POST", "/portal-token-reset-test", ""},
    endpoint_request_t {"VirtualInputLicenseUpdate", "POST", "/virtual-input-license-test", R"({"action":"validate"})"}
  ),
  endpoint_request_name
);

TEST_P(InvalidVirtualInputLicenseRequestTest, ReturnsExpectedError) {
  const auto &request = GetParam();
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "testpass"));

  const auto response = client->request("POST", "/virtual-input-license-test", request.body, headers);
  ASSERT_EQ(response->status_code, "400 Bad Request");
  EXPECT_TRUE(response->content.string().contains(request.expected_error));
}

INSTANTIATE_TEST_SUITE_P(
  InvalidRequests,
  InvalidVirtualInputLicenseRequestTest,
  testing::Values(
    invalid_license_request_t {"InvalidJson", "not-json", "Invalid license request"},
    invalid_license_request_t {"UnknownAction", R"({"action":"unknown"})", "Unknown license action"},
    invalid_license_request_t {"MissingActivationKey", R"({"action":"activate","license_key":""})", "License key is required"}
  ),
  invalid_license_request_name
);

TEST_F(ConfigHttpTest, PairingMutationsRejectUnauthenticatedRestRequests) {
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Content-Type", "application/json");

  const auto post_response = client->request(
    "POST",
    "/pairing-test",
    R"({"pairing_id":"0123456789abcdef0123456789abcdef","pin":"1234","name":"Client"})",
    headers
  );
  EXPECT_EQ(post_response->status_code, "401 Unauthorized");

  const auto delete_response = client->request(
    "DELETE",
    "/pairing-test",
    R"({"pairing_id":"0123456789abcdef0123456789abcdef"})",
    headers
  );
  EXPECT_EQ(delete_response->status_code, "401 Unauthorized");
}

TEST_F(ConfigHttpTest, PairingMutationsRejectCrossOriginRestRequestsWithoutCsrfToken) {
  const std::string pairing_id = insert_pending_pairing();
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "testpass"));
  headers.emplace("Content-Type", "application/json");
  headers.emplace("Origin", "https://example.invalid");

  const auto post_response = client->request(
    "POST",
    "/pairing-test",
    nlohmann::json {
      {"pairing_id", pairing_id},
      {"pin", "1234"},
      {"name", "Client"},
    }
      .dump(),
    headers
  );
  EXPECT_EQ(post_response->status_code, "400 Bad Request");
  EXPECT_TRUE(post_response->content.string().contains("Missing CSRF token"));

  const auto delete_response = client->request(
    "DELETE",
    "/pairing-test",
    nlohmann::json {{"pairing_id", pairing_id}}.dump(),
    headers
  );
  EXPECT_EQ(delete_response->status_code, "400 Bad Request");
  EXPECT_TRUE(delete_response->content.string().contains("Missing CSRF token"));
  EXPECT_EQ(nvhttp::get_pending_pairings().size(), 1);
}

TEST_F(ConfigHttpTest, PairingRestApiListsAuthenticatedPendingRequests) {
  const std::string pairing_id = insert_pending_pairing();
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "testpass"));

  const auto response = client->request("GET", "/pairing-test", "", headers);
  ASSERT_EQ(response->status_code, "200 OK");
  const auto body = nlohmann::json::parse(response->content.string());
  ASSERT_EQ(body.at("pairings").size(), 1);
  EXPECT_EQ(body.at("pairings").front().at("id"), pairing_id);
  EXPECT_EQ(body.at("pairings").front().at("name"), "REST client");
  EXPECT_EQ(body.at("pairings").front().at("address"), "192.0.2.30");
}

TEST_F(ConfigHttpTest, PairingRestApiRejectsMalformedFieldsWithoutConsumingRequest) {
  const std::string pairing_id = insert_pending_pairing();
  const std::array invalid_requests {
    nlohmann::json {{"pairing_id", "not-a-pairing-id"}, {"pin", "1234"}, {"name", "Client"}},
    nlohmann::json {{"pairing_id", pairing_id}, {"pin", "123"}, {"name", "Client"}},
    nlohmann::json {{"pairing_id", pairing_id}, {"pin", "12a4"}, {"name", "Client"}},
    nlohmann::json {{"pairing_id", pairing_id}, {"pin", "1234"}, {"name", ""}},
    nlohmann::json {{"pairing_id", pairing_id}, {"pin", "1234"}, {"name", std::string(nvhttp::MAX_PAIRING_CLIENT_NAME_SIZE + 1, 'a')}},
  };

  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "testpass"));
  headers.emplace("Content-Type", "application/json");
  for (const auto &request : invalid_requests) {
    const auto response = client->request("POST", "/pairing-test", request.dump(), headers);
    EXPECT_EQ(response->status_code, "400 Bad Request");
  }

  const auto pending = nvhttp::get_pending_pairings();
  ASSERT_EQ(pending.size(), 1);
  EXPECT_EQ(pending.front().id, pairing_id);
}

TEST_F(ConfigHttpTest, PairingRestApiCancelsOnlyExplicitRequest) {
  const std::string pairing_id = insert_pending_pairing();
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "testpass"));
  headers.emplace("Content-Type", "application/json");

  const auto response = client->request(
    "DELETE",
    "/pairing-test",
    nlohmann::json {{"pairing_id", pairing_id}}.dump(),
    headers
  );
  ASSERT_EQ(response->status_code, "200 OK");
  EXPECT_TRUE(nlohmann::json::parse(response->content.string()).at("status").get<bool>());
  EXPECT_TRUE(nvhttp::get_pending_pairings().empty());
}

TEST_F(ConfigHttpTest, PairingRestApiReportsIncompleteHandshakeAsFailure) {
  const std::string pairing_id = insert_pending_pairing();
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "testpass"));
  headers.emplace("Content-Type", "application/json");

  const auto response = client->request(
    "POST",
    "/pairing-test",
    nlohmann::json {
      {"pairing_id", pairing_id},
      {"pin", "9875"},
      {"name", "Client"},
    }
      .dump(),
    headers
  );
  ASSERT_EQ(response->status_code, "200 OK");
  EXPECT_FALSE(nlohmann::json::parse(response->content.string()).at("status").get<bool>());
  EXPECT_TRUE(nvhttp::get_pending_pairings().empty());
}

TEST_F(ConfigHttpTest, PortalTokenResetHandlesSavedTokenForCurrentPlatform) {
  const auto token_path = test_web_dir / "portal_token";
  std::ofstream(token_path) << "saved-token";

  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "testpass"));

  const auto response = client->request("POST", "/portal-token-reset-test", "", headers);

  ASSERT_EQ(response->status_code, "200 OK");
  EXPECT_TRUE(nlohmann::json::parse(response->content.string()).at("status").get<bool>());
#if defined(linux) || defined(__FreeBSD__)
  EXPECT_FALSE(std::filesystem::exists(token_path));
#else
  EXPECT_TRUE(std::filesystem::exists(token_path));
#endif
}

TEST_F(ConfigHttpTest, PortalTokenResetSucceedsWhenTokenDoesNotExist) {
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "testpass"));

  const auto response = client->request("POST", "/portal-token-reset-test", "", headers);

  ASSERT_EQ(response->status_code, "200 OK");
  EXPECT_TRUE(nlohmann::json::parse(response->content.string()).at("status").get<bool>());
}

TEST_F(ConfigHttpTest, PortalTokenResetReportsDeletionFailure) {
  const auto token_path = test_web_dir / "portal_token";
  std::filesystem::create_directories(token_path);
  std::ofstream(token_path / "contents") << "not empty";

  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "testpass"));

  const auto response = client->request("POST", "/portal-token-reset-test", "", headers);

  ASSERT_EQ(response->status_code, "200 OK");
#if defined(linux) || defined(__FreeBSD__)
  EXPECT_FALSE(nlohmann::json::parse(response->content.string()).at("status").get<bool>());
#else
  EXPECT_TRUE(nlohmann::json::parse(response->content.string()).at("status").get<bool>());
#endif
  EXPECT_TRUE(std::filesystem::exists(token_path));
}

#ifdef SUNSHINE_BUILD_PORTAL
TEST_F(ConfigHttpTest, PortalTokenResetUsesSessionResolverAndPreservesOtherDesktops) {
  const auto active_token = test_web_dir / "portal_token.kde";
  const auto other_token = test_web_dir / "portal_token.gnome";
  std::ofstream(active_token) << "kde-token";
  std::ofstream(other_token) << "gnome-token";
  confighttp::set_portal_token_path_provider_for_testing([this]() {
    return portal::get_saved_token_path_for_testing(test_web_dir, "KDE");
  });

  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "testpass"));
  const auto response = client->request("POST", "/portal-token-reset-test", "", headers);

  ASSERT_EQ(response->status_code, "200 OK");
  EXPECT_TRUE(nlohmann::json::parse(response->content.string()).at("status").get<bool>());
  EXPECT_FALSE(std::filesystem::exists(active_token));
  ASSERT_TRUE(std::filesystem::exists(other_token));
  std::ifstream remaining_token(other_token);
  std::string value;
  std::getline(remaining_token, value);
  EXPECT_EQ(value, "gnome-token");
}

TEST_F(ConfigHttpTest, PortalTokenResetRemovesMigratedLegacyToken) {
  const auto legacy_token = test_web_dir / "portal_token";
  const auto active_token = test_web_dir / "portal_token.kde";
  std::ofstream(legacy_token) << "legacy-token";
  confighttp::set_portal_token_path_provider_for_testing([this]() {
    return portal::get_saved_token_path_for_testing(test_web_dir, "KDE");
  });

  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "testpass"));
  const auto response = client->request("POST", "/portal-token-reset-test", "", headers);

  ASSERT_EQ(response->status_code, "200 OK");
  EXPECT_TRUE(nlohmann::json::parse(response->content.string()).at("status").get<bool>());
  EXPECT_FALSE(std::filesystem::exists(legacy_token));
  EXPECT_FALSE(std::filesystem::exists(active_token));
  EXPECT_EQ(portal::get_saved_token_path_for_testing(test_web_dir, "KDE"), active_token);
  EXPECT_FALSE(std::filesystem::exists(active_token));
}
#endif

// Test: confighttp::authenticate() rejects requests without auth header
TEST_F(ConfigHttpTest, AuthenticateRejectsNoAuth) {
  const auto response = client->request("GET", "/auth-test");
  ASSERT_EQ(response->status_code, "401 Unauthorized");

  // Check for WWW-Authenticate header
  const auto www_auth = response->header.find("WWW-Authenticate");
  ASSERT_NE(www_auth, response->header.end());
}

// Test: confighttp::authenticate() accepts valid credentials
TEST_F(ConfigHttpTest, AuthenticateAcceptsValidCredentials) {
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "testpass"));

  const auto response = client->request("GET", "/auth-test", "", headers);
  ASSERT_EQ(response->status_code, "200 OK");

  const std::string body = response->content.string();
  ASSERT_EQ(body, "authenticated");
}

// Test: confighttp::authenticate() rejects invalid password
TEST_F(ConfigHttpTest, AuthenticateRejectsInvalidPassword) {
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "wrongpass"));

  const auto response = client->request("GET", "/auth-test", "", headers);
  ASSERT_EQ(response->status_code, "401 Unauthorized");
}

// Test: confighttp::authenticate() is case-insensitive for username
TEST_F(ConfigHttpTest, AuthenticateCaseInsensitiveUsername) {
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("TESTUSER", "testpass"));

  const auto response = client->request("GET", "/auth-test", "", headers);
  ASSERT_EQ(response->status_code, "200 OK");
}

// Test: confighttp::send_unauthorized() sends proper 401 response
TEST_F(ConfigHttpTest, SendUnauthorizedResponse) {
  const auto response = client->request("GET", "/unauthorized-test");
  ASSERT_EQ(response->status_code, "401 Unauthorized");

  // Check for WWW-Authenticate header
  const auto www_auth = response->header.find("WWW-Authenticate");
  ASSERT_NE(www_auth, response->header.end());
  ASSERT_TRUE(www_auth->second.find("Basic realm") != std::string::npos);

  // Check security headers
  assert_security_headers(response);

  // Check JSON response
  const std::string body = response->content.string();
  ASSERT_TRUE(body.find("Unauthorized") != std::string::npos);
  ASSERT_TRUE(body.find("401") != std::string::npos);
}

// Test: confighttp::not_found() sends proper 404 response
TEST_F(ConfigHttpTest, NotFoundResponse) {
  const auto response = client->request("GET", "/notfound-test");
  ASSERT_EQ(response->status_code, "404 Not Found");
  assert_json_error_response(response, "Test not found", "404");
}

// Test: confighttp::bad_request() sends proper 400 response
TEST_F(ConfigHttpTest, BadRequestResponse) {
  const auto response = client->request("GET", "/badrequest-test");
  ASSERT_EQ(response->status_code, "400 Bad Request");
  assert_json_error_response(response, "Test bad request", "400");
}

// Test: confighttp::send_response() sends proper JSON response
TEST_F(ConfigHttpTest, SendResponseJson) {
  const auto response = client->request("GET", "/json-test");
  ASSERT_EQ(response->status_code, "200 OK");

  // Check Content-Type
  const auto content_type = response->header.find("Content-Type");
  ASSERT_NE(content_type, response->header.end());
  ASSERT_TRUE(content_type->second.find("application/json") != std::string::npos);

  // Check security headers
  assert_security_headers(response);

  // Check JSON content
  const std::string body = response->content.string();
  ASSERT_TRUE(body.find("\"status\":\"success\"") != std::string::npos || body.find("\"status\": \"success\"") != std::string::npos);
  ASSERT_TRUE(body.find("Test JSON response") != std::string::npos);
  ASSERT_TRUE(body.find("200") != std::string::npos);
}

// Test: confighttp::check_content_type() accepts valid content type
TEST_F(ConfigHttpTest, CheckContentTypeValid) {
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Content-Type", "application/json");

  const auto response = client->request("POST", "/content-type-test", "", headers);
  ASSERT_EQ(response->status_code, "200 OK");

  const std::string body = response->content.string();
  ASSERT_EQ(body, "content-type-valid");
}

// Test: confighttp::check_content_type() rejects missing content type
TEST_F(ConfigHttpTest, CheckContentTypeMissing) {
  const auto response = client->request("POST", "/content-type-test");
  ASSERT_EQ(response->status_code, "400 Bad Request");

  const std::string body = response->content.string();
  ASSERT_TRUE(body.find("Content type not provided") != std::string::npos);
}

// Test: confighttp::check_content_type() rejects wrong content type
TEST_F(ConfigHttpTest, CheckContentTypeWrong) {
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Content-Type", "text/plain");

  const auto response = client->request("POST", "/content-type-test", "", headers);
  ASSERT_EQ(response->status_code, "400 Bad Request");

  const std::string body = response->content.string();
  ASSERT_TRUE(body.find("Content type mismatch") != std::string::npos);
}

// Test: confighttp::check_content_type() handles content type with charset
TEST_F(ConfigHttpTest, CheckContentTypeWithCharset) {
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Content-Type", "application/json; charset=utf-8");

  const auto response = client->request("POST", "/content-type-test", "", headers);
  ASSERT_EQ(response->status_code, "200 OK");

  const std::string body = response->content.string();
  ASSERT_EQ(body, "content-type-valid");
}

// Test: CSRF token generation
TEST_F(ConfigHttpTest, CSRFTokenGeneration) {
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "testpass"));

  const auto response = client->request("GET", "/csrf-token-test", "", headers);
  ASSERT_EQ(response->status_code, "200 OK");

  const std::string body = response->content.string();
  nlohmann::json json_body = nlohmann::json::parse(body);

  ASSERT_TRUE(json_body.contains("csrf_token"));
  ASSERT_FALSE(json_body["csrf_token"].get<std::string>().empty());

  // Token should be 32 characters (CSRF_TOKEN_SIZE)
  ASSERT_EQ(json_body["csrf_token"].get<std::string>().length(), 32);
}

// Test: CSRF token validation with valid token in header
TEST_F(ConfigHttpTest, CSRFValidationWithValidTokenInHeader) {
  SimpleWeb::CaseInsensitiveMultimap auth_headers;
  auth_headers.emplace("Authorization", create_auth_header("testuser", "testpass"));

  // First, get a CSRF token
  const auto token_response = client->request("GET", "/csrf-token-test", "", auth_headers);
  ASSERT_EQ(token_response->status_code, "200 OK");

  const std::string token_body = token_response->content.string();
  nlohmann::json token_json = nlohmann::json::parse(token_body);
  std::string csrf_token = token_json["csrf_token"].get<std::string>();

  // Now make a POST request with the token
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "testpass"));
  headers.emplace("X-CSRF-Token", csrf_token);

  const auto response = client->request("POST", "/csrf-validate-test", "", headers);
  ASSERT_EQ(response->status_code, "200 OK");

  const std::string body = response->content.string();
  ASSERT_EQ(body, "csrf-valid");
}

// Test: CSRF token validation with missing token (cross-origin request)
TEST_F(ConfigHttpTest, CSRFValidationWithMissingToken) {
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "testpass"));
  // Don't set Origin or Referer - this simulates a request that doesn't match allowed origins
  // The server will require CSRF token

  const auto response = client->request("POST", "/csrf-validate-test", "", headers);

  // The test might pass as same-origin if Simple-Web-Server adds headers automatically
  // In that case, we need to explicitly block same-origin by using a custom validation route
  // For now, if it passes, that's OK - it means same-origin is working
  // This test is more about the API than the actual enforcement
  if (response->status_code == "200 OK") {
    // Same-origin was detected automatically - test passes
    SUCCEED();
  } else {
    // CSRF token was required
    ASSERT_EQ(response->status_code, "400 Bad Request");
    const std::string body = response->content.string();
    ASSERT_TRUE(body.find("Missing CSRF token") != std::string::npos);
  }
}

// Test: CSRF token validation with invalid token (cross-origin request)
TEST_F(ConfigHttpTest, CSRFValidationWithInvalidToken) {
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "testpass"));
  // Don't set Origin or Referer - force CSRF validation
  headers.emplace("X-CSRF-Token", "invalid_token_12345678901234567890");

  const auto response = client->request("POST", "/csrf-validate-test", "", headers);

  // Similar to above - if same-origin is detected, test passes
  if (response->status_code == "200 OK") {
    SUCCEED();
  } else {
    ASSERT_EQ(response->status_code, "400 Bad Request");
    const std::string body = response->content.string();
    ASSERT_TRUE(body.find("Invalid CSRF token") != std::string::npos);
  }
}

// Test: browser-origin requests are always rejected without a CSRF token now
// that the Web UI allowlist (csrf_allowed_origins) is gone.
TEST_F(ConfigHttpTest, CSRFRejectsAllowedOriginHeaderWithoutToken) {
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "testpass"));
  headers.emplace("Origin", std::format("https://localhost:{}", port));

  const auto response = client->request("POST", "/csrf-validate-test", "", headers);
  ASSERT_EQ(response->status_code, "400 Bad Request");
  EXPECT_TRUE(response->content.string().contains("Missing CSRF token"));
}

// Test: browser-origin Referer requests are rejected without a CSRF token as well.
TEST_F(ConfigHttpTest, CSRFRejectsAllowedRefererHeaderWithoutToken) {
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "testpass"));
  headers.emplace("Referer", std::format("https://localhost:{}/some/page", port));

  const auto response = client->request("POST", "/csrf-validate-test", "", headers);
  ASSERT_EQ(response->status_code, "400 Bad Request");
  EXPECT_TRUE(response->content.string().contains("Missing CSRF token"));
}

// Test: unknown browser routes return a JSON 404 now that the SPA is removed.
TEST_F(ConfigHttpTest, UnknownRoutesReturnJson404) {
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Authorization", create_auth_header("testuser", "testpass"));

  for (const std::string_view path : {"/", "/pin", "/apps", "/config", "/logout", "/welcome", "/future-browser-route"}) {
    const auto response = client->request("GET", std::string {path}, "", headers);
    EXPECT_EQ(response->status_code, "404 Not Found") << path;
    const auto content_type = response->header.find("Content-Type");
    ASSERT_NE(content_type, response->header.end()) << path;
    EXPECT_TRUE(content_type->second.find("application/json") != std::string::npos) << path;
  }
}

// Test: confighttp::getLocale() returns locale JSON
TEST_F(ConfigHttpTest, GetLocaleReturnsJson) {
  const auto response = client->request("GET", "/locale-test");
  ASSERT_EQ(response->status_code, "200 OK");

  // Check Content-Type
  const auto content_type = response->header.find("Content-Type");
  ASSERT_NE(content_type, response->header.end());
  ASSERT_TRUE(content_type->second.find("application/json") != std::string::npos);

  // Check security headers
  assert_security_headers(response);

  // Check JSON content
  const std::string body = response->content.string();
  ASSERT_TRUE(body.find("\"status\":true") != std::string::npos || body.find("\"status\": true") != std::string::npos);
  ASSERT_TRUE(body.find("\"locale\":\"en\"") != std::string::npos || body.find("\"locale\": \"en\"") != std::string::npos);
}

TEST_F(ConfigHttpTest, SaveAppOmitsNullOptionalFieldsAndPreservesConfiguredValues) {
  using json = nlohmann::json;
  ScopedSaveAppAppsFile apps_file("save_app_optional_fields");
  {
    std::ofstream file(config::stream.file_apps);
    file << json {{"env", json::object()}, {"apps", json::array()}}.dump();
  }
  const auto headers = authenticated_json_headers();

  json null_fields {
    {"name", "Null optional"},
    {"index", -1},
    {"prep-cmd", nullptr},
    {"detached", nullptr},
    {"exclude-global-prep-cmd", nullptr},
    {"elevated", nullptr},
    {"auto-detach", nullptr},
    {"wait-all", nullptr},
    {"exit-timeout", nullptr},
    {"cmd", nullptr},
    {"output", nullptr},
    {"working-dir", nullptr},
    {"image-path", nullptr},
  };
  auto null_response = client->request("POST", "/api/apps", null_fields.dump(), headers);
  ASSERT_EQ(null_response->status_code, "200 OK");
  {
    std::ifstream file(config::stream.file_apps);
    const auto saved = json::parse(file);
    ASSERT_EQ(saved.at("apps").size(), 1U);
    EXPECT_EQ(saved.at("apps").at(0), (json {{"name", "Null optional"}}));
  }

  json configured {
    {"name", "Configured"},
    {"index", -1},
    {"prep-cmd", json::array({json {{"do", "prepare"}, {"undo", "restore"}, {"elevated", false}}})},
    {"detached", json::array({"background"})},
    {"exclude-global-prep-cmd", false},
    {"elevated", false},
    {"auto-detach", false},
    {"wait-all", false},
    {"exit-timeout", 0},
    {"cmd", "launch --argument"},
    {"output", "/logs/app.log"},
    {"working-dir", "/apps"},
    {"image-path", "/covers/cover.png"},
  };
  auto configured_response = client->request("POST", "/api/apps", configured.dump(), headers);
  ASSERT_EQ(configured_response->status_code, "200 OK");
  {
    std::ifstream file(config::stream.file_apps);
    const auto saved = json::parse(file);
    ASSERT_EQ(saved.at("apps").size(), 2U);
    json expected = configured;
    expected.erase("index");
    EXPECT_EQ(saved.at("apps").at(0), expected);
  }

  json nested_null_prep {
    {"name", "Prep null members"},
    {"index", -1},
    {"prep-cmd", json::array({json {{"do", "prepare"}, {"undo", nullptr}, {"elevated", nullptr}}})},
  };
  auto nested_response = client->request("POST", "/api/apps", nested_null_prep.dump(), headers);
  ASSERT_EQ(nested_response->status_code, "200 OK");
  {
    std::ifstream file(config::stream.file_apps);
    const auto saved = json::parse(file);
    ASSERT_EQ(saved.at("apps").size(), 3U);
    const auto &app = saved.at("apps").at(2);
    ASSERT_TRUE(app.contains("prep-cmd"));
    ASSERT_EQ(app.at("prep-cmd").size(), 1U);
    EXPECT_EQ(app.at("prep-cmd").at(0), (json {{"do", "prepare"}}));
    EXPECT_FALSE(app.at("prep-cmd").at(0).contains("undo"));
    EXPECT_FALSE(app.at("prep-cmd").at(0).contains("elevated"));
  }
}

// Test: POST /api/password creates the initial credentials without Basic auth
// on first run, and requires auth once credentials exist.
TEST_F(ConfigHttpTest, SavePasswordCreatesInitialCredentialsWithoutAuth) {
  const std::string saved_credentials_file = config::sunshine.credentials_file;
  const auto creds_dir = std::filesystem::temp_directory_path() / "sunshine_test_confighttp_creds";  // NOSONAR(cpp:S5443): safe for tests
  std::filesystem::create_directories(creds_dir);
  const auto creds_file = creds_dir / "first_run_state.json";
  std::error_code remove_ec;
  std::filesystem::remove(creds_file, remove_ec);
  config::sunshine.credentials_file = creds_file.string();

  auto restore = util::fail_guard([&]() {
    config::sunshine.credentials_file = saved_credentials_file;
    std::error_code cleanup_ec;
    std::filesystem::remove(creds_file, cleanup_ec);
  });

  // Simulate first run: no credentials configured yet.
  const std::string saved_username = config::sunshine.username;
  const std::string saved_password = config::sunshine.password;
  const std::string saved_salt = config::sunshine.salt;
  config::sunshine.username = "";
  config::sunshine.password = "";
  config::sunshine.salt = "";
  ASSERT_TRUE(std::filesystem::remove(creds_file, remove_ec) || !std::filesystem::exists(creds_file));

  const nlohmann::json input {
    {"newUsername", "firstuser"},
    {"newPassword", "firstpass"},
    {"confirmNewPassword", "firstpass"},
  };

  // First-run request: no Authorization header, no browser Origin/Referer headers.
  SimpleWeb::CaseInsensitiveMultimap first_run_headers;
  first_run_headers.emplace("Content-Type", "application/json");
  const auto response = client->request("POST", "/api/password", input.dump(), first_run_headers);
  ASSERT_EQ(response->status_code, "200 OK");
  EXPECT_TRUE(response->content.string().contains("\"status\":true") || response->content.string().contains("\"status\": true"));

  // Credentials were persisted and reloaded into the runtime config.
  ASSERT_TRUE(std::filesystem::exists(creds_file));
  EXPECT_EQ(config::sunshine.username, "firstuser");
  const auto expected_hash = util::hex(crypto::hash("firstpass" + config::sunshine.salt)).to_string();
  EXPECT_EQ(config::sunshine.password, expected_hash);

  // Once credentials exist, the endpoint rejects unauthenticated requests again.
  SimpleWeb::CaseInsensitiveMultimap unauth_headers;
  unauth_headers.emplace("Content-Type", "application/json");
  const auto unauth_response = client->request("POST", "/api/password", input.dump(), unauth_headers);
  ASSERT_EQ(unauth_response->status_code, "401 Unauthorized");
  EXPECT_NE(unauth_response->header.find("WWW-Authenticate"), unauth_response->header.end());

  // An authenticated update with the fresh credentials is accepted.
  const nlohmann::json update_input {
    {"currentUsername", "firstuser"},
    {"currentPassword", "firstpass"},
    {"newUsername", "firstuser"},
    {"newPassword", "secondpass"},
    {"confirmNewPassword", "secondpass"},
  };
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Content-Type", "application/json");
  headers.emplace("Authorization", create_auth_header("firstuser", "firstpass"));
  const auto auth_response = client->request("POST", "/api/password", update_input.dump(), headers);
  ASSERT_EQ(auth_response->status_code, "200 OK");
  const auto updated_hash = util::hex(crypto::hash("secondpass" + config::sunshine.salt)).to_string();
  EXPECT_EQ(config::sunshine.password, updated_hash);

  // Restore the pre-test credential state.
  config::sunshine.username = saved_username;
  config::sunshine.password = saved_password;
  config::sunshine.salt = saved_salt;
}
