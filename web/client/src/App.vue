<script setup lang="ts">

import { RouterLink, RouterView } from 'vue-router'
import AppSidebar from '@/components/AppSidebar.vue'
import {
	SidebarProvider,
	SidebarTrigger,
	SidebarInset,
} from '@/components/ui/sidebar'
import { useDark } from '@vueuse/core'

useDark()

</script>

<template>
	<SidebarProvider>
		<AppSidebar />

		<SidebarInset>
			<!-- <header class="fixed"> -->
			<!-- 	<SidebarTrigger /> -->
			<!-- </header> -->

			<RouterView />
		</SidebarInset>
	</SidebarProvider>
</template>
