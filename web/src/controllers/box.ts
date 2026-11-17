import { type Request, type Response } from 'express';
import { Writable } from 'stream';
// import path from 'path';

type Position = {
	x: number,
	y: number,
	z: number
};

let box_position: Position = {
    x: 0,
    y: 0,
    z: 0
};

const setPosition = async (req: Request, res: Response) => {
	console.log(req.body);

	// const stream = new Writable({
	// 	write(chunk, encoding, callback) {
	// 		console.log(chunk.toString());
	// 		callback();
	// 	}
	// });

	res.json({"status": "ok"});
};

const stream = async (req: Request, res: Response) => {
};

export { setPosition, stream };
