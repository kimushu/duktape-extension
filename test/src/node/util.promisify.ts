const util = require("util");

describe("util.promisify()", () => {

	it("is a function", () => {
		assert.isFunction(util.promisify);
	});

	it("throws when non-function specified", () => {
		let test = 1;
		assert.throws(() => util.promisify(test));
	})

	it("returns another function", () => {
		let test = () => {};
		let result = util.promisify(test);
		assert.isFunction(result);
		assert.notStrictEqual(result, test);
	});

	xit("length is preserved", () => {
		let test = (a, b) => {};
		assert.equal(util.promisify(test).length, test.length);
	});

});
