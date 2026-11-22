import express, { type Router } from 'express';
import { router as boxRoute } from './box.ts';

const router: Router = express.Router();

const apiRoutes = [
	{
		path: '/box',
		route: boxRoute
	}
];

apiRoutes.forEach((route) => {
	router.use(route.path, route.route);
});

export { router };
