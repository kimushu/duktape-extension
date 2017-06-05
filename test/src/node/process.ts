"use strict";

describe("process", () => {
    it("exists", () => assert.exists(process));

    describe("arch property", () => {
        it("is a string", () => assert.isString(process.arch));
        it("is readonly", () => {
            assert.throws(() => (<any>process).arch = "foo");
        });
    });

    describe("exit()", () => {
        it("is a function", () => assert.isFunction(process.exit));
    });

    describe("exitCode property", () => {
        it("exists", () => assert.property(process, "exitCode"));
        it("is writable", () => {
            assert.doesNotThrow(() => process.exitCode = 0);
        });
        it("is a number", () => assert.isNumber(process.exitCode));
    });

    describe("nextTick()", () => {
        it("is a function", () => assert.isFunction(process.nextTick));
        it("invokes callback with correct arguments", (done) => {
            process.nextTick((a, b, c) => {
                try {
                    assert.strictEqual(a, 123);
                    assert.strictEqual(b, "foo");
                    done();
                } catch (error) {
                    done(error);
                }
            }, 123, "foo");
        });
        it("invokes all callbacks in queued order", (done) => {
            let i = 1;
            process.nextTick(() => {
                if (i !== 1) { done(`invalid order ${i}`); }
                ++i;
            });
            process.nextTick(() => {
                if (i !== 2) { done(`invalid order ${i}`); }
                ++i;
                done();
            });
        });
    });

    describe("version property", () => {
        it("is a string", () => assert.isString(process.version));
        it("is readonly", () => {
            assert.throws(() => (<any>process).version = "foo");
        });
    });

    describe("versions property", () => {
        it("exists", () => assert.exists(process.versions));

        it("has duktape property", () => {
            assert.strictEqual(`v${process.versions.duktape}`, process.version);
        })

        it("has dux property", () => {
            assert.isString(process.versions.dux);
        })
    });
});