describe("console", () => {
	const DUMMY = "DUMMY MESSAGE! THIS MESSAGE CAN BE IGNORED";

	it("exists", () => {
		assert.exists(console);
	});

	describe("log", () => {
		it("is a function", () => {
			assert.isFunction(console.log);
		});

		it("returns undefined", () => {
			assert.isUndefined(console.log(DUMMY));
		});

		it("throws", () => {
			assert.throws(() => {
				throw [];
			}, Array);
		});
	});

	describe("info", () => {
		it("is a function", () => {
			assert.isFunction(console.info);
		});

		it("returns undefined", () => {
			assert.isUndefined(console.info(DUMMY));
		});
	});

	describe("warn", () => {
		it("is a function", () => {
			assert.isFunction(console.warn);
		});

		it("returns undefined", () => {
			assert.isUndefined(console.warn(DUMMY));
		});
	});

	describe("error", () => {
		it("is a function", () => {
			assert.isFunction(console.error);
		});

		it("returns undefined", () => {
			assert.isUndefined(console.error(DUMMY));
		});
	});
});
