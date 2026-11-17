<script setup lang="ts">
	import { BoxIcon, Gamepad2Icon } from '@lucide/vue'
	import {
		Sidebar,
		SidebarContent,
		SidebarGroup,
		SidebarGroupLabel,
		SidebarMenu,
		SidebarMenuButton,
        SidebarMenuItem,
        SidebarRail
	} from '@/components/ui/sidebar'
	import { RouterLink } from 'vue-router';

	const items = [
		{
			title: 'Box',
			url: '/',
			icon: BoxIcon,
		},
		{
			title: 'Controller',
			url: '/controller',
			icon: Gamepad2Icon,
		}
	]
</script>

<template>
	<Sidebar>
		<SidebarContent>
			<SidebarGroup>
				<SidebarGroupLabel>ESP32 App</SidebarGroupLabel>
			</SidebarGroup>
			<SidebarMenu>
				<SidebarMenuItem v-for="item in items" :key="item.title">
					<sidebarMenuButton as-child>
						<RouterLink :to="item.url">
						<component :is="item.icon" />
						<span>{{ item.title }}</span>
						</RouterLink>
					</SidebarMenuButton>
				</SidebarMenuItem>
			</SidebarMenu>
		</SidebarContent>

		<SidebarRail />
	</Sidebar>
</template>
