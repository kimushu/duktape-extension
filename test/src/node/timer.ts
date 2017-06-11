describe("timers", () => {
    describe("setTimeout()", () => {
        it("is a function", () => assert.isFunction(setTimeout));
        it("throws TypeError if callback is not callable", () => {
            assert.throws(() => setTimeout(null, 0), TypeError);
        });
        it("throws TypeError if delay is a number", () => {
            assert.throws(() => setTimeout(() => {}, null), TypeError);
        });
        it("returns an object with ref()&unref() and invokes callback with correct arguments", (done) => {
            let timeout = setTimeout((a, b, c) => {
                try {
                    assert.strictEqual(a, 123);
                    assert.strictEqual(b, "foo");
                    assert.isUndefined(c);
                    done();
                } catch (error) {
                    done(error);
                }
            }, 0, 123, "foo");
            assert.isFunction(timeout.ref);
            assert.isFunction(timeout.unref);
        });
        it("invokes callbacks in expiration order", (done) => {
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
        it("unref() returns undefined and timeout breaks event-loop", (done) => {
            let i = 0;
            let timeout = setTimeout(() => {
                (function(){ delete this.espresso_done; })();
                if (i === 1) {
                    done();
                } else {
                    done("failed to detect event-loop break");
                }
            }, 200);
            assert.isUndefined(timeout.unref());
            (function(){ this.espresso_done = () => { i = 1; }; })();
        });
        it("2nd call of unref() is ignored", (done) => {
            let i = 0;
            let timeout = setTimeout(() => {
                (function(){ delete this.espresso_done; })();
                if (i === 0) {
                    done();
                } else {
                    done("event-loop incorrectly finished");
                }
            }, 200);
            assert.isUndefined(timeout.unref());    // 1st
            assert.isUndefined(timeout.unref());    // 2nd
            assert.isUndefined(timeout.ref());
            (function(){ this.espresso_done = () => { i = 1; }; })();
        });
    });
    describe("clearTimeout()", () => {
        it("is a function", () => assert.isFunction(clearTimeout));
        it("throws TypeError if timeout is not valid", () => {
            assert.throws(() => clearTimeout(null), TypeError);
        });
        it("throws RangeError if id does not exist", () => {
            assert.throws(() => clearTimeout(<any>{}), RangeError);
        });
        it("stops timer before callback is invoked", (done) => {
            let timeout = setTimeout(() => {
                done(new Error("triggered"));
            }, 200);
            setTimeout(() => {
                clearTimeout(timeout);
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
        it("returns an object with ref() & unref() and invokes callback multiple times with correct arguments", (done) => {
            let repeat = 0;
            let timeout = setInterval((a, b, c) => {
                try {
                    assert.strictEqual(a, 456);
                    assert.strictEqual(b, "bar");
                    assert.isUndefined(c);
                    ++repeat;
                } catch (error) {
                    done(error);
                }
            }, 100, 456, "bar");
            assert.isFunction(timeout.ref);
            assert.isFunction(timeout.unref);
            setTimeout(() => {
                clearInterval(timeout);
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
