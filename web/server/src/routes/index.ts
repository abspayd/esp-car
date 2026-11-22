import express, { type Router } from 'express';
import {router as apiRoute } from './api.ts';

const router: Router = express.Router();

const defaultRoutes = [
	{
		path: '/api',
		route: apiRoute
	}
];

defaultRoutes.forEach((route) => {
	router.use(route.path, route.route);
});

export { router };
