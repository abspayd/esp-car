import express, { type Express, } from 'express';
import { router as routes } from './routes/index.ts';
import path from 'path';

const app: Express = express();
const port = 3000;

app.set('views', path.join(import.meta.dirname, './views'));
app.set('view engine', 'ejs');

app.use(express.json());

app.use('/', routes);
app.listen(port);
