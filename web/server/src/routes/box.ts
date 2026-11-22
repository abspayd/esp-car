import express, { type Router } from 'express';
import * as boxController from '../controllers/box.ts';

const router: Router = express.Router();

router
	.route('/data')
	.get(boxController.stream);

router.route('/position')
	.post(boxController.setPosition);

export { router };
