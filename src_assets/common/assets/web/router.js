import { createRouter, createWebHistory } from 'vue-router'

/**
 * Web UI routes rendered from the single Vite entry page.
 *
 * PIN pairing is the home page; applications/featured/welcome pages are
 * removed from this build.
 */
const routes = [
  { path: '/', redirect: '/pin' },
  { path: '/pin', component: () => import('./Pin.vue') },
  { path: '/config', component: () => import('./Config.vue') },
  { path: '/troubleshooting', component: () => import('./Troubleshooting.vue') },
  { path: '/:pathMatch(.*)*', redirect: '/pin' },
]

const router = createRouter({
  history: createWebHistory(),
  routes,
  scrollBehavior(to) {
    return to.hash ? { el: to.hash } : { top: 0 }
  },
})

export default router
