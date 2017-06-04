const util = require("util");

describe("util.promisify()", () => {

	it("is a function", () => {
		assert.isFunction(util.promisify);
	});

	it("returns Promise", () => {
		let test = () => {};
		assert.instanceOf(util.promisify(test), Promise);
	});

});
