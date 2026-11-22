<script setup lang="ts">
	import { onMounted, useTemplateRef, onUnmounted } from 'vue'
	import {
		Scene,
		PerspectiveCamera,
		WebGLRenderer,
		BoxGeometry,
		MeshStandardMaterial,
		Mesh,
		AmbientLight,
		DirectionalLight,
	} from 'three'

	const canvasContainer = useTemplateRef('canvasContainer');
	let renderer: WebGLRenderer, camera: PerspectiveCamera;

	let resizeObserver: ResizeObserver|null = null;

	const websocket = new WebSocket('/socket');

	onMounted(() => {
		if (!canvasContainer.value) {
			return;
		}

		const width = canvasContainer.value.offsetWidth;
		const height = canvasContainer.value.offsetHeight;

		if (width === undefined ||
			height === undefined) {
			return;
		}

		const scene = new Scene();
		camera = new PerspectiveCamera(75, width / height, 0.1, 1000);

		renderer = new WebGLRenderer();
		renderer.setSize(width, height);

		renderer.render(scene, camera);

		renderer.domElement.style.width = '100%';
		renderer.domElement.style.height = '100%';
		renderer.domElement.style.display = 'block';

		canvasContainer.value.appendChild(renderer.domElement);

		const geometry = new BoxGeometry(2, .2, 1);
		const material = new MeshStandardMaterial({ color: 0x00ff00 });
		const cube = new Mesh(geometry, material);

		const light = new AmbientLight( 0x404040 );
		const directionalLight = new DirectionalLight(0xffffff, 0.9);

		scene.add(light);
		scene.add(directionalLight);
		scene.add(cube);
		camera.position.z = 5;

		const animate = (time: DOMHighResTimeStamp) => {
			// cube.rotation.x = time / 2000;
			// cube.rotation.y = time / 1000;

			renderer.render(scene, camera);
		}

		renderer.setAnimationLoop(animate);

		resizeObserver = new ResizeObserver((entries) => {
			for (const entry of entries) {
				const w = entry.contentRect.width;
				const h = entry.contentRect.height;

				if (w > 0 && h > 0) {
					renderer.setSize(w, h, false);
					camera.aspect = w / h;
					camera.updateProjectionMatrix()
				}
			}
		});

		resizeObserver.observe(canvasContainer.value);

		websocket.addEventListener('open', () => {
			console.log('Websocket opened');
		});

		websocket.addEventListener('message', (ev) => {
			
			try {
				const data = JSON.parse(ev.data);

				cube.position.x = data.x;
				cube.position.y = data.y;
				cube.position.z = data.z;

			} catch (e) {
				console.log("Unable to parse payload");
			}
		})
	});

	onUnmounted(() => {
		if (resizeObserver && canvasContainer.value) {
			resizeObserver.unobserve(canvasContainer.value);
			resizeObserver.disconnect();
		}
		if (renderer) {
			renderer.dispose();
		}

		websocket.addEventListener('close', () => {
			console.log('Websocket closed');
		});
	})

</script>

<template>
	<div class="flex-1 w-full h-full max-h-dvh overflow-hidden" ref="canvasContainer"></div>
</template>
