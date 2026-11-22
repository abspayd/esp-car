import type { Position } from "./types/box.ts";

class StateStore {
    state: Map<any, any>;

	constructor() {
		this.state = new Map();
	}

	setPosition(coords: Position) {
		this.state.set('position', {
			...coords,
			timestamp: Date.now()
		});
	}

	getPosition() {
		return this.state.get('position');
	}
}

const store = new StateStore();
export { store };
