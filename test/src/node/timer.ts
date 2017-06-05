describe("timers", () => {
    describe("setTimeout()", () => {
        it("is a function", () => assert.isFunction(setTimeout));
        it("throws TypeError if callback is not callable", () => {
            assert.throws(() => setTimeout(null, 0), TypeError);
        });
        it("throws TypeError if delay is a number", () => {
            assert.throws(() => setTimeout(() => {}, null), TypeError);
        });
        it("returns a number and invokes callback with correct arguments", (done) => {
            let id = setTimeout((a, b, c) => {
                try {
                    assert.strictEqual(a, 123);
                    assert.strictEqual(b, "foo");
                    assert.isUndefined(c);
                    done();
                } catch (error) {
                    done(error);
                }
            }, 0, 123, "foo");
            assert.isNumber(id);
        });
        it("invokes callback in expiration order", (done) => {
            let i = 0;
            setTimeout(() => {
                try {
                    assert.strictEqual(i, 0);
                    i += 1;
                } catch (error) {
                    done(error);
                }
            }, 100);
            setTimeout(() => {
                try {
                    assert.strictEqual(i, 3);
                    i += 4;
                    done();
                } catch (error) {
                    done(error);
                }
            }, 300);
            setTimeout(() => {
                try {
                    assert.strictEqual(i, 1);
                    i += 2;
                } catch (error) {
                    done(error);
                }
            }, 200);
        });
    });
    describe("clearTimeout()", () => {
        it("is a function", () => assert.isFunction(clearTimeout));
        it("throws TypeError if id is not a number", () => {
            assert.throws(() => clearTimeout(null), TypeError);
        });
        it("throws RangeError if id does not exist", () => {
            assert.throws(() => clearTimeout(0), RangeError);
        });
        it("stops timer before callback is invoked", (done) => {
            let id = setTimeout(() => {
                done(new Error("triggered"));
            }, 200);
            setTimeout(() => {
                clearTimeout(id);
            }, 100);
            setTimeout(() => {
                done();
            }, 300);
        });
    });
    describe("setInterval()", () => {
        it("is a function", () => assert.isFunction(setInterval));
        it("throws TypeError if callback is not callable", () => {
            assert.throws(() => setInterval(null, 0), TypeError);
        });
        it("throws TypeError if delay is a number", () => {
            assert.throws(() => setInterval(() => {}, null), TypeError);
        });
        it("returns a number and invokes callback multiple times with correct arguments", (done) => {
            let repeat = 0;
            let id = setInterval((a, b, c) => {
                try {
                    assert.strictEqual(a, 456);
                    assert.strictEqual(b, "bar");
                    assert.isUndefined(c);
                    ++repeat;
                } catch (error) {
                    done(error);
                }
            }, 100, 456, "bar");
            assert.isNumber(id);
            setTimeout(() => {
                clearInterval(id);
                try {
                    assert.strictEqual(repeat, 3);
                } catch (error) {
                    done(error);
                }
                setTimeout(() => {
                    try {
                        assert.strictEqual(repeat, 3);
                        done();
                    } catch (error) {
                        done(error);
                    }
                }, 100);
            }, 350);
        });
    });
});
