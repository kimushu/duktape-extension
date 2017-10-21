import * as delay from "delay";

describe("delay", () => {
    it("is a function", () => {
        assert.isFunction(delay);
    });

    it("returns Promise object", (done) => {
        let promise = delay(0);
        assert.instanceOf(promise, Promise);
        promise.then(() => done());
    });

    let delayCheck = (done, generator, catcher) => {
        let state = 0;
        const timeout = 10;
        const value = {abc: 123};
        setTimeout(() => { state = 1; }, timeout - 1);
        let promise = generator(timeout, value);
        promise[catcher]((result) => {
            assert.equal(result, value);
            if (state === 1) {
                state = 2;
            }
        });
        setTimeout(() => {
            if (state === 2) {
                return done();
            }
            done(new Error("unexpected call order"));
        }, timeout + 1);
    };

    it("fulfills with correct value after specified time", (done) => {
        delayCheck(done, (timeout, value) => delay(timeout, value), "then");
    });

    describe("reject", () => {
        it("is a function", () => {
            assert.isFunction(delay.reject);
        });

        it("returns Promise object", (done) => {
            let promise = delay.reject(0);
            assert.instanceOf(promise, Promise);
            promise.catch(() => done());
        });

        it("rejects with correct reason after specified time", (done) => {
            delayCheck(done, (timeout, value) => delay.reject(timeout, value), "catch");
        });
    });
});
