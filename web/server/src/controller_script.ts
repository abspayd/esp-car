import process from "node:process";
import WebSocket, { WebSocketServer } from 'ws';

type Position = {
	x: number,
	y: number,
	z: number
};

let stdin = process.stdin;

stdin.setRawMode(true);

stdin.resume();

stdin.setEncoding('utf-8');

let connected: boolean = false;

let pos: Position = {
	x: 0,
	y: 0,
	z: 0,
};

const wss = new WebSocketServer({ port: 3001, clientTracking: true });

const debounce = (callback: () => void, delay: number) => {
	let timeout: NodeJS.Timeout;
	return function(...args: []) {
		clearTimeout(timeout);
		timeout = setTimeout(() => {
			callback.apply(args);
		}, delay);
	};
};


const velocity = 0.1;

stdin.on('data', async function (key) {

	if (key === '\u0003') {
		process.exit();
	}

	if (key === 'a') {
		pos.x -= velocity;
	}
	if (key === 'd') {
		pos.x += velocity;
	}
	if (key === 'e') {
		pos.y -= velocity;
	}
	if (key === 'q') {
		pos.y += velocity;
	}
	if (key === 's') {
		pos.z += velocity;
	}
	if (key === 'w') {
		pos.z -= velocity;
	}

	if (wss.clients.size > 0) {
		wss.clients.forEach((client) => {
			client.send(JSON.stringify(pos));
		})
	}

	await fetch('http://127.0.0.1:3000/box/position', {
		method: 'POST',
		body: JSON.stringify(pos),
		headers: {
			"Content-Type": "application/json"
		}
	});
});
