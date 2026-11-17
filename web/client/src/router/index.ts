import { createRouter, createWebHistory } from 'vue-router'

const router = createRouter({
	history: createWebHistory(import.meta.env.BASE_URL),
	routes: [
		{
			path: '/',
			name: 'Box',
			component: () => import('../views/BoxView.vue')
		},
		{
			path: '/controller',
			name: 'Controller',
			component: () => import('../views/ControllerView.vue')
		}
	],
})

export default router
