const fs = require('fs');
const path = require('path');
const { name, version } = require(path.join('..', 'package.json'));
const tar = require('tar');
const modname = `${name}-${version}`;
const tgzname = `${modname}.tar.gz`;

tar.create({
    gzip: true,
    prefix: modname,
    file: tgzname
}, [
    'dist',
    'README.md',
    'LICENSE',
])
.then(() => {
    console.log('Release package generated:', tgzname);
})
.catch((reason) => {
    console.error(reason);
    process.exitCode = 1;
});
