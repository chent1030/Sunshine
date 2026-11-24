# API

CATL投屏 exposes a JSON configuration API on the loopback interface only
(`https://127.0.0.1:47990`). The browser Web UI has been removed; this API is
meant to be consumed by the native desktop client running on the same machine.

Unless otherwise specified, authentication is required for all API calls. You can
authenticate using basic authentication with the admin username and password.

## First-Run Credentials

Two equivalent paths create the initial admin credentials:

1. **CLI**: `sunshine <username> <password>` writes the credential file and exits.
2. **API**: `POST /api/password` is reachable **without** authentication while no
   credentials exist (the listener only accepts loopback connections). Once
   credentials exist, the endpoint requires basic authentication like every other
   route.

```bash
# First run: set credentials over the loopback API (no auth yet)
curl -k -X POST https://127.0.0.1:47990/api/password \
  -H "Content-Type: application/json" \
  -d '{"newUsername":"admin","newPassword":"secret","confirmNewPassword":"secret"}'

# Every later call authenticates with Basic auth
curl -k -u admin:secret https://127.0.0.1:47990/api/config
```

## CSRF Protection

State-changing API endpoints (POST, DELETE) are protected against Cross-Site
Request Forgery (CSRF) attacks.

**For Non-Browser Applications:**
- Non-browser clients (e.g. `curl`, scripts, the desktop client) are **exempt**
  from CSRF protection
- Requests with no `Origin` or `Referer` header (as is typical for non-browser
  clients) are automatically allowed without a CSRF token

**For Browser Requests:**
- The Web UI was removed and the `csrf_allowed_origins` allowlist no longer
  exists. Any request that carries an `Origin` or `Referer` header is treated as
  browser-initiated and is rejected unless it presents a valid CSRF token. No
  endpoint issues tokens anymore, so browsers are effectively blocked from
  driving the API.

## Endpoints

## GET /api/apps
@copydoc confighttp::getApps()

## POST /api/apps
@copydoc confighttp::saveApp()

## POST /api/apps/close
@copydoc confighttp::closeApp()

## DELETE /api/apps/{index}
@copydoc confighttp::deleteApp()

## GET /api/clients/list
@copydoc confighttp::getClients()

## POST /api/clients/unpair
@copydoc confighttp::unpair()

## POST /api/clients/unpair-all
@copydoc confighttp::unpairAll()

## POST /api/clients/update
@copydoc confighttp::updateClient()

## GET /api/config
@copydoc confighttp::getConfig()

## GET /api/configLocale
@copydoc confighttp::getLocale()

## POST /api/config
@copydoc confighttp::saveConfig()

## GET /api/logs
@copydoc confighttp::getLogs()

## POST /api/password
@copydoc confighttp::savePassword()

## POST /api/pin
@copydoc confighttp::savePin()

## POST /api/reset-display-device-persistence
@copydoc confighttp::resetDisplayDevicePersistence()

## POST /api/reset-portal-token
@copydoc confighttp::resetPortalToken()

## POST /api/restart
@copydoc confighttp::restart()

## GET /api/virtual-input/status
@copydoc confighttp::getVirtualInputStatus()
