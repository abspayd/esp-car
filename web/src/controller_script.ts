import process from "node:process";

type Position = {
	x: number,
	y: number,
	z: number
};

let stdin = process.stdin;

stdin.setRawMode(true);

stdin.resume();

stdin.setEncoding('utf-8');

let pos: Position = {
	x: 0,
	y: 0,
	z: 0,
};

const debounce = (callback: () => void, delay: number) => {
	let timeout: NodeJS.Timeout;
	return function(...args: []) {
		clearTimeout(timeout);
		timeout = setTimeout(() => {
			callback.apply(args);
		}, delay);
	};
};

stdin.on('data', async function (key) {


	if (key === '\u0003') {
		process.exit();
	}

	if (key === 'w') {
		pos.x--;
	}
	if (key === 's') {
		pos.x++;
	}
	if (key === 'a') {
		pos.y--;
	}
	if (key === 'd') {
		pos.y++;
	}
	if (key === 'e') {
		pos.z++;
	}
	if (key === 'q') {
		pos.z--;
	}

	await fetch('http://127.0.0.1:3000/box/position', {
		method: 'POST',
		body: JSON.stringify(pos),
		headers: {
			"Content-Type": "application/json"
		}
	});
});
