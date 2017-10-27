describe("Modules", () => {
    it("has global require() function", () => {
        let global: {require: Dux.RequireFunction} = (function(){ return this; })();
        assert.isFunction(global.require);
    });
    it("has global module object <repl>", () => {
        let global: {module: Dux.Module} = (function(){ return this; })();
        assert.exists(global.module);
        assert.equal(global.module.id, "<repl>");
        assert.isUndefined(global.module.parent);
        assert.isNull(global.module.filename);
    });
    it("can load core module", () => {
        let mod: Dux.Console = require("console");
        assert.exists(mod);
    });
    it("can load module (with .js extension)", () => {
        let mod = require("/mod1.js");
        assert.equal(mod.self, "mod1.js");
    });
    it("can load JSON object (with .json extension)", () => {
        let obj = require("/mod1.json");
        assert.deepEqual(obj, {self: "mod1.json"});
    });
    it("can load module (without .js extension)", () => {
        let mod = require("/mod1");
        assert.equal(mod.self, "mod1");
    });
    it("can load module (.js fallback)", () => {
        let mod = require("/mod2");
        assert.equal(mod.self, "mod2.js");
    });
    it("can load JSON object (.json fallback)", () => {
        let obj = require("/json1");
        assert.deepEqual(obj, {self: "json1.json"});
    });
    it("can load nested module with relative path", () => {
        let mod = require("/sub/mod3");
        assert.equal(mod.self, "mod3.js");
        assert.equal(mod.sub.self, "mod2.js");
    });
    it("throws SyntaxError when module has a syntax error", () => {
        assert.throws(() => require("/mod4"), SyntaxError);
    });
    it("caches loaded module", () => {
        let mod_a = require("/mod1.js");
        mod_a.modified = true;
        let mod_b = require("/sub/../mod1.js");
        assert.equal(mod_b.modified, true);
    });
    it("throws Error when module not found", () => {
        assert.throws(() => require("/not_exists"), Error);
    });
});
