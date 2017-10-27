describe("Modules", () => {
    it("has require() function", () => {
        assert.isFunction(require);
    });
    it("has global module object for REPL", () => {
        let mod;
        (() => mod = this.module)();
        assert.exists(mod);
        console.log(mod.id);
        assert.equal(mod.id, "<repl>");
        assert.isUndefined(mod.parent);
        assert.isNull(mod.filename);
    });
});