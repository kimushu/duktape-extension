import * as path from "path";

describe("Path", () => {
    describe("basename()", () => {
        it("is a function with 2 arguments", () => {
            assert.isFunction(path.basename);
            assert.equal(path.basename.length, 2);
        });
        it("throws TypeError when called with non-string path", () => {
            assert.throws(() => path.basename(<any>123), TypeError);
        });
        it("throws TypeError when called with non-string ext", () => {
            assert.throws(() => path.basename("foo", <any>123), TypeError);
        });
        it("returns path if path does not contain separator", () => {
            assert.equal(path.basename("foobar.txt"), "foobar.txt");
        });
        it("returns path without ext if path does not contain separator", () => {
            assert.equal(path.basename("foobar.txt", ".txt"), "foobar");
        });
        it("returns basename if path contains separator", () => {
            assert.equal(path.basename("/foo/bar.txt"), "bar.txt");
        });
        it("returns basename without ext if path contains separator", () => {
            assert.equal(path.basename("/foo/bar.txt", ".txt"), "bar");
        });
    });
    describe("delimiter", () => {
        it("is \":\"", () => {
            assert.equal(path.delimiter, ":");
        });
        it("is a read-only property", () => {
            assert.throws(() => (<any>path).delimiter = ":");
        });
    });
    describe("dirname()", () => {
        it("is a function with 1 argument", () => {
            assert.isFunction(path.dirname);
            assert.equal(path.dirname.length, 1);
        });
        it("throws TypeError when called with non-string", () => {
            assert.throws(() => path.dirname(<any>123), TypeError);
        });
        it("returns \".\" for path without directory", () => {
            assert.equal(path.dirname("abc"), ".");
        });
        it("returns \".\" for empty path", () => {
            assert.equal(path.dirname(""), ".");
        });
        it("returns \"/\" for path on root directory", () => {
            assert.equal(path.dirname("/foo"), "/");
        });
        it("returns \"/\" for root directory", () => {
            assert.equal(path.dirname("/"), "/");
        });
        it("returns correct path for relative path", () => {
            assert.equal(path.dirname("foo/bar/baz"), "foo/bar");
        });
        it("returns correct path for absolute path", () => {
            assert.equal(path.dirname("/foo/bar/baz"), "/foo/bar");
        });
    });
    describe("normalize()", () => {
        it("is a function with 1 argument", () => {
            assert.isFunction(path.normalize);
            assert.equal(path.normalize.length, 1);
        });
        it("throws TypeError when called with non-string", () => {
            assert.throws(() => {
                path.normalize(<any>123);
            }, TypeError);
        });
        it("removes \".\" from path", () => {
            assert.equal(path.normalize("/foo/./bar"), "/foo/bar");
            assert.equal(path.normalize("/foo/./bar/"), "/foo/bar/");
        });
        it("removes last \".\" from path", () => {
            assert.equal(path.normalize("/foo/bar/."), "/foo/bar");
            assert.equal(path.normalize("/foo/bar/./"), "/foo/bar/");
        });
        it("removes \"..\" and move to parent", () => {
            assert.equal(path.normalize("/foo/../bar"), "/bar");
            assert.equal(path.normalize("/foo/../bar/"), "/bar/");
        });
        it("removes last \"..\" and move to parent", () => {
            assert.equal(path.normalize("/foo/bar/.."), "/foo");
            assert.equal(path.normalize("/foo/bar/../"), "/foo/");
        });
        it("does not follow upper dir from root", () => {
            assert.equal(path.normalize("/foo/../../bar/../baz"), "/baz");
            assert.equal(path.normalize("/foo/../../bar/../baz/"), "/baz/");
        });
        it("follows upper dir for relative path", () => {
            assert.equal(path.normalize("foo/../../bar/../baz"), "../baz");
            assert.equal(path.normalize("foo/../../bar/../baz/"), "../baz/");
        });
    });
    describe("posix", () => {
        it("is a circular reference to path", () => {
            assert.equal(path.posix, path);
        });
        it("is a read-only property", () => {
            assert.throws(() => (<any>path).posix = path);
        });
    });
    describe("sep", () => {
        it("is \"/\"", () => {
            assert.equal(path.sep, "/");
        });
        it("is a read-only property", () => {
            assert.throws(() => (<any>path).sep = "/");
        });
    });
});
