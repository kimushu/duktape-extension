describe("Promise", () => {
    it("exists", () => {
        assert.exists(Promise);
    });

    it("is a constructor function with one argument", () => {
        assert.isFunction(Promise);
        assert.equal(Promise.length, 1);
    });

    it("invokes executor with two functions before constructor finishes", () => {
        let ok = false;
        let p = new Promise((...args: Function[]) => {
            assert.equal(args.length, 2);
            assert.isFunction(args[0]);
            assert.equal(args[0].length, 1);
            assert.isFunction(args[1]);
            assert.equal(args[1].length, 1);
            ok = true;
        });
        assert.isTrue(ok);
    });

    it("has resolve() method which has one argument", () => {
        assert.isFunction(Promise.resolve);
        assert.equal(Promise.resolve.length, 1);
    });

    it("resolve() fulfills after current tick", (done) => {
        let value = {abc: 12345};
        let tick = 0;
        Promise.resolve(value)
        .then((result) => {
            if (value !== result) {
                return done("incorrect value");
            }
            if (tick !== 1) {
                return done("incorrect tick");
            }
            done();
        }, (reason) => {
            done("incorrect rejection");
        });
        tick = 1;
    });

    it("has reject() method which has one argument", () => {
        assert.isFunction(Promise.reject);
        assert.equal(Promise.reject.length, 1);
    });

    it("reject() rejects after current tick", (done) => {
        let value = {def: 54321};
        let tick = 0;
        Promise.reject(value)
        .then((result) => {
            done("incorrect fulfillment");
        }, (reason) => {
            if (value !== reason) {
                return done("incorrect reason");
            }
            if (tick !== 1) {
                return done("incorrect tick");
            }
            done();
        });
        tick = 1;
    });

    it("has all() method which has one argument", () => {
        assert.isFunction(Promise.all);
        assert.equal(Promise.all.length, 1);
    });

    it("has race() method which has one argument", () => {
        assert.isFunction(Promise.race);
        assert.equal(Promise.race.length, 1);
    });

    it("instance has then() method which has two arguments", () => {
        let p = new Promise(() => {});
        assert.isFunction(p.then);
        assert.equal(p.then.length, 2);
    });

    it("instance has catch() method which has one argument", () => {
        let p = new Promise(() => {});
        assert.isFunction(p.catch);
        assert.equal(p.catch.length, 1);
    });
});
