const util = require("util");

describe("util", () => {
	describe("promisify()", () => {
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

		it("calls original function with correct arguments", (done) => {
			let test = (a, b, c, d) => {
				try {
					assert.strictEqual(a, 1);
					assert.strictEqual(b, undefined);
					assert.isFunction(c);
					assert.isUndefined(d);
					done();
				} catch (error) {
					done(error);
				}
			};
			util.promisify(test)(1, undefined);
		});

		it("fulfills with correct value", () => {
			let test = (value, cb) => cb(undefined, value);
			return assert.becomes(util.promisify(test)(123), 123);
		});

		it("rejects with correct reason", () => {
			let test = (value, cb) => cb(new Error("foo"));
			return assert.isRejected(util.promisify(test)(123), Error, "Error: foo");
		});

		it("returns func[util.promisify.custom] if it is defined", () => {
			let test = () => {};
			let inner = () => {};
			test[util.promisify.custom] = inner;
			assert.strictEqual(util.promisify(test), inner);
		});
	});
});
