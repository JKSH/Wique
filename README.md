Wique
=====
Wique (pronounced "Week") downloads wiki articles and some associated metadata
to disk for fast local access. It was originally created to support work on the
[Qt Wiki](https://wiki.qt.io), but it can be easily adapted to support any
public MediaWiki-based site.

**WARNING**: Wique currently attempts to download all pages from the wiki. Do
not use it on large wikis, like Wikipedia!


Use Cases
---------
- Sort and filter the list of page titles in the wiki.
- Save raw wiki text to disk for searching, grepping, etc.
- Identify redirected articles.


Building the Program
--------------------
Open wique.pro in any IDE that supports qmake (Qt Creator 3.x is recommended),
and build it with the default settings.

Requirements:
- Qt 5.0 or later
- A C++11 compliant compiler
