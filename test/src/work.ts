describe("work", () => {
    let queue_work_caller: (buf: Uint8Array, callback: (...args: any[]) => void, ...args: any[]) => void;
    queue_work_caller = (function(){return this})().__queue_work_caller;
    it("starts worker thread and invoke callbacks", (done) => {
        let buf = new Uint8Array([10, 0]);
        queue_work_caller(buf, (result, ...args) => {
            try {
                assert.equal(result, 10);
                assert.equal(args.length, 2);
                assert.equal(args[0], "abc");
                assert.equal(args[1], 123);
                assert.equal(buf[1], 0);
                process.nextTick(() => {
                    try {
                        assert.equal(buf[1], 1);
                    } catch (reason) {
                        done(reason);
                    }
                    done();
                });
            } catch (reason) {
                done(reason);
            }
        }, "abc", 123);
    });
    it("starts multiple workers and invoke callbacks in correct order", (done) => {
        let buf1 = new Uint8Array([200, 0]);
        let buf2 = new Uint8Array([100, 0]);
        let order = 0;
        queue_work_caller(buf1, (result, ...args) => {
            try {
                assert.equal(order, 3);
                assert.equal(result, 200);
                assert.equal(args.length, 0);
                order = 4;
                process.nextTick(() => {
                    try {
                        assert.equal(buf1[1], 1);
                        order = 5;
                    } catch (reason) {
                        done(reason);
                    }
                    done();
                });
            } catch (reason) {
                done(reason);
            }
        });
        queue_work_caller(buf2, (result, ...args) => {
            try {
                assert.equal(order, 1);
                assert.equal(result, 100);
                assert.equal(args.length, 0);
                order = 2;
                process.nextTick(() => {
                    try {
                        assert.equal(buf2[1], 1);
                        order = 3;
                    } catch (reason) {
                        done(reason);
                    }
                });
            } catch (reason) {
                done(reason);
            }
        });
        order = 1;
    });
});
