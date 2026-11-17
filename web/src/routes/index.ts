import express, { type Router } from 'express';
import './box.ts';
import { router as boxRoute } from './box.ts';

const router: Router = express.Router();

const defaultRoutes = [
	{
		path: '/box',
		route: boxRoute
	}
];

defaultRoutes.forEach((route) => {
	router.use(route.path, route.route);
});

export { router };
