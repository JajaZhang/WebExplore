# WebExplore
This project is creating a web browser, such as a simpler version of Chrome.

The project consists three components: a router, a controller and multiple web tabs. 

Language: C

Key feature: 

1. The project has sufficient protection on unexpected user input, such as handling when the either of the router and the controller gets killed unexpectedly. Then the web explorer is able to clean up all the processes and collect all garbages.

2. Between the processes, there are pipes for inter process communications. 

3. The browser allows up to 100 tabs.

4. You can input the whole complete link in the tab to perform an actual search.

Improvement:

1. Improve a smarter search

2. Make a better UI

Motivation:

The project is originally from an operating system class, but it is a very interesting experience learning and writing this simple brower.