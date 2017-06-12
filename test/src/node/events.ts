import * as EventEmitter from "events";

describe("Events", () => {
	it("exists", () => {
		assert.exists(EventEmitter);
	});
    it("has circular reference", () => {
        assert.strictEqual(EventEmitter, EventEmitter.EventEmitter);
    });
    it("has defaultMaxListeners property with default value 10", () => {
        assert.strictEqual(EventEmitter.defaultMaxListeners, 10);
    });
    it("can be constructed without argument", () => {
        assert.instanceOf(new EventEmitter(), EventEmitter);
    });
    describe("getMaxListeners()", () => {
        it("returns undefined for new EventEmitter", () => {
            let e = new EventEmitter();
            assert.isUndefined(e.getMaxListeners());
        });
    });
    describe("setMaxListeners()", () => {
        [
            ["positive number", 5],
            ["infinity", Infinity],
            ["zero", 0],
            ["negative number", -5, true],
            ["undefined", undefined, true],
            ["string", "1", true],
        ].forEach((set) => {
            const [ desc, newValue, throws ] = set;
            if (throws) {
                it(`throws TypeError when ${desc} is given`, () => {
                    let e = new EventEmitter();
                    assert.throws(() => e.setMaxListeners(<any>newValue), TypeError);
                });
            } else {
                it(`is chainable and can be verified by getMaxListeners() for ${desc}`, () => {
                    let e = new EventEmitter();
                    assert.strictEqual(e.setMaxListeners(<any>newValue), e);
                    assert.strictEqual(e.getMaxListeners(), newValue);
                });
            }
        });
    });
    describe("eventNames()", () => {
        it("returns empty array for new EventEmitter", () => {
            let e = new EventEmitter();
            let result = e.eventNames();
            assert.instanceOf(result, Array);
            assert.strictEqual(result.length, 0);
        });
    });
    describe("listenerCount()", () => {
        it("returns zero for non-existent event", () => {
            let e = new EventEmitter();
            assert.strictEqual(e.listenerCount(null), 0);
        });
    });
    ["addListener", "on", "once", "prependListener", "prependOnceListener"].forEach((method) => {
        describe(`${method}()`, () => {
            it("succeeds to register 1 listener and is chainable", () => {
                let e = new EventEmitter();
                assert.strictEqual(e[method]("foo", () => {}), e);
                assert.strictEqual(e.eventNames().length, 1);
                assert.strictEqual(e.listenerCount("foo"), 1);
            });
            it("succeeds to register 2 listeners for the same event and is chainable", () => {
                let e = new EventEmitter();
                assert.strictEqual(e[method]("foo", () => {}), e);
                assert.strictEqual(e[method]("foo", () => {}), e);
                assert.strictEqual(e.eventNames().length, 1);
                assert.strictEqual(e.listenerCount("foo"), 2);
            });
            it("succeeds to register 2 listeners for the different events and is chainable", () => {
                let e = new EventEmitter();
                assert.strictEqual(e[method]("foo", () => {}), e);
                assert.strictEqual(e[method]("bar", () => {}), e);
                assert.strictEqual(e.eventNames().length, 2);
                assert.strictEqual(e.listenerCount("foo"), 1);
                assert.strictEqual(e.listenerCount("bar"), 1);
            });
        });
    });
});
