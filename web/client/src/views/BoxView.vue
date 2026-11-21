<script setup lang="ts">
	import Box from '@/components/Box.vue'
	import { onMounted, onUnmounted } from 'vue'

	const websocket = new WebSocket('/api/box/data');

	onMounted(() => {
		websocket.addEventListener('open', () => {
			console.log('Connected');

			// TODO
		});
	});

	onUnmounted(() => {
		websocket.addEventListener('close', () => {
			console.log('disconnected');
		});
	});

</script>

<template>
	<div class="flex flex-col min-h-dvh w-full">
		<Box />
	</div>
</template>
