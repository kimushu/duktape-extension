import * as sprintf from "sprintf";
import * as sprintfJs from "sprintf-js";

describe("sprintf", () => {
    it("exists", () => {
        assert.exists(sprintf);
    });

    it("has sprintf() function", () => {
        assert.isFunction(sprintf.sprintf);
    });

    it("has vsprintf() function", () => {
        assert.isFunction(sprintf.vsprintf);
    });

    it("returns string", () => {
        assert.isString(sprintf.sprintf(""));
    });

    it("plaintext", () => {
        assert.equal(sprintf.sprintf("text"), "text");
    });

    it("plaintext with %+05d (standard printf format)", () => {
        let format = "foo%+05dbar";
        let answer = "foo+0123bar";
        assert.equal(sprintf.sprintf(format, 123), answer);
        assert.equal(sprintf.vsprintf(format, [123]), answer);
    });

    it("plaintext with %+*d (standard printf format with *)", () => {
        let format = "foo%+*dbar";
        let answer = "foo +123bar";
        assert.equal(sprintf.sprintf(format, 5, 123), answer);
        assert.equal(sprintf.vsprintf(format, [5, 123]), answer);
    });

    it("plaintext with %-10b (binary extension)", () => {
        let format = "foo%-10bbar";
        let answer = "foo1111011   bar";
        assert.equal(sprintf.sprintf(format, 123), answer);
        assert.equal(sprintf.vsprintf(format, [123]), answer);
    });

    it("plaintext with %0*b (binary extension with *)", () => {
        let format = "foo%0*bbar";
        let answer = "foo0001111011bar";
        assert.equal(sprintf.sprintf(format, 10, 123), answer);
        assert.equal(sprintf.vsprintf(format, [10, 123]), answer);
    });

    it("%T (type extension)", () => {
        assert.equal(sprintf.sprintf("%T", 123), "number");
        assert.equal(sprintf.sprintf("%T", null), "null");
        assert.equal(sprintf.sprintf("%T", undefined), "undefined");
        assert.equal(sprintf.sprintf("%T", () => {}), "function");
        assert.equal(sprintf.sprintf("%T", []), "array");
        assert.equal(sprintf.sprintf("%T", {}), "object");
        assert.equal(sprintf.sprintf("%T", ""), "string");
    });

    it("%j (JSON extension)", () => {
        assert.equal(sprintf.sprintf("%j", {a: 123, b: undefined}), '{"a":123}');
    });
});

describe("sprintf-js", () => {
    it("exists", () => {
        assert.exists(sprintfJs);
    });

    it("has sprintf() function as an alias of sprintf.sprintf()", () => {
        assert.equal(sprintfJs.sprintf, sprintf.sprintf);
    });

    it("has vsprintf() function as an alias of sprintf.vsprintf()", () => {
        assert.equal(sprintfJs.vsprintf, sprintf.vsprintf);
    });
});
