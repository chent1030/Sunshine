<template>
  <div>
    <nav class="navbar navbar-expand-lg navbar-sunshine">
      <div class="container-fluid">
        <RouterLink class="navbar-brand" to="/" title="CATL投屏">
          <img src="/images/logo-sunshine-45.png" height="45" alt="CATL投屏">
        </RouterLink>
        <button class="navbar-toggler" type="button" data-bs-toggle="collapse" data-bs-target="#navbarSupportedContent"
                aria-controls="navbarSupportedContent" aria-expanded="false" aria-label="Toggle navigation">
          <span class="navbar-toggler-icon"></span>
        </button>
        <div class="collapse navbar-collapse" id="navbarSupportedContent">
          <ul class="navbar-nav me-auto mb-2 mb-lg-0">
            <li class="nav-item">
              <RouterLink class="nav-link" to="/pin">
                <Lock :size="18" class="icon"></Lock>
                {{ $t('navbar.pin') }}
              </RouterLink>
            </li>
            <li class="nav-item">
              <RouterLink class="nav-link" to="/config">
                <Settings :size="18" class="icon"></Settings>
                {{ $t('navbar.configuration') }}
              </RouterLink>
            </li>
            <li class="nav-item">
              <RouterLink class="nav-link" to="/troubleshooting">
                <Info :size="18" class="icon"></Info>
                {{ $t('navbar.troubleshoot') }}
              </RouterLink>
            </li>
          </ul>
          <ul class="navbar-nav ms-auto mb-2 mb-lg-0">
            <li class="nav-item">
              <ThemeToggle/>
            </li>
          </ul>
        </div>
      </div>
    </nav>
    <Notification></Notification>
  </div>
</template>

<script>
import { Info, Lock, Settings } from '@lucide/vue'
import ThemeToggle from './ThemeToggle.vue'
import Notification from './Notification.vue'

export default {
  components: {
    ThemeToggle,
    Notification,
    Lock,
    Settings,
    Info
  }
}
</script>

<style>
/** @brief Keep the Sunshine toggler icon above Bootstrap's light and dark variants. */
.navbar-sunshine .navbar-toggler .navbar-toggler-icon {
  --bs-navbar-toggler-icon-bg: url("data:image/svg+xml,%3csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 30 30'%3e%3cpath stroke='rgba%28255, 255, 255, 0.9%29' stroke-linecap='round' stroke-miterlimit='10' stroke-width='2' d='M4 7h22M4 15h22M4 23h22'/%3e%3c/svg%3e");
}
</style>
