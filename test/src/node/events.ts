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
    describe("emit()", () => {
        it("returns true and invokes listeners in correct order", () => {
            let e = new EventEmitter();
            let seq = 0;
            let e1 = {x: "hoge"};
            let e2 = 123;
            function check(args: any[]) {
                return args.length === 2 && args[0] === e1 && args[1] === e2;
            }
            e.addListener    ("foo", (...args) => { seq = (check(args) && seq === 1) ? 2 : -1; });
            e.prependListener("foo", (...args) => { seq = (check(args) && seq === 0) ? 1 : -1; });
            e.on             ("foo", (...args) => { seq = (check(args) && seq === 2) ? 3 : -1; });
            e.on             ("fooa", () => { seq = -1; });
            assert.isTrue(e.emit("foo", e1, e2));
            assert.strictEqual(seq, 3);
        });
        it("returns false when no listener called", () => {
            let e = new EventEmitter();
            let seq = 0;
            e.on("foo", () => { seq = -1; })
            assert.isFalse(e.emit("fooa"));
            assert.strictEqual(seq, 0);
        })
        it("is not affected by removing/adding listener in callback", () => {
            let e = new EventEmitter();
            let seq = 0;
            let f2 = () => { seq = (seq === 1) ? 2 : -1; };
            let f3 = () => { seq = -1; };
            let f1 = () => {
                seq = (seq === 0) ? 1 : -1;
                e.removeListener("bar", f2);
                e.on("bar", f3);
            };
            e.on("bar", f1);
            e.on("bar", f2);
            assert.isTrue(e.emit("bar"));
            assert.strictEqual(seq, 2);
        });
        it("is not affected by removing event in callback", () => {
            let e = new EventEmitter();
            let seq = 0;
            e.on("bar", () => {
                seq = (seq === 0) ? 1 : -1;
                e.removeAllListeners("bar");
            });
            e.on("bar", () => {
                seq = (seq === 1) ? 2 : -1;
            });
            assert.isTrue(e.emit("bar"));
            assert.strictEqual(seq, 2);
        });
        it("is not affected by removing all events in callback", () => {
            let e = new EventEmitter();
            let seq = 0;
            e.on("bar", () => {
                seq = (seq === 0) ? 1 : -1;
                e.removeAllListeners();
            });
            e.on("bar", () => {
                seq = (seq === 1) ? 2 : -1;
            });
            assert.isTrue(e.emit("bar"));
            assert.strictEqual(seq, 2);
        });
    });
});
