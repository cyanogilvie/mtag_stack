MTAG_STACK
==========

This tiny library provides a data structure for efficiently building and
resolving a trie of int64_t offsets.  The motivation is to provide an mtag
implementation compatible with re2c that is as performant as possible (low
allocation load, low stack mem usage, good cache locality).

LICENSE
-------
This package is placed in the public domain: the author disclaims
copyright and liability to the extent allowed by law. For those
jurisdictions that limit an author’s ability to disclaim copyright this
package can be used under the terms of the CC0, BSD, or MIT licenses. No
attribution, permission or fees are required to use this for whatever 
you like, commercial or otherwise, though I would urge its users to do
good and not evil to the world.
