# Summary

## Project 3

Date: 29 Nov 2024

State: Finish all basic functionalities as well as the second option which is multi-dimension array.

In the duration of writing code, I refer to [this repo](https://github.dev/Unkrible/Compilers/tree/master/Project3). Thanks Unkrible!

Compared with the last project where I used a myriad of `malloc` to manually create data. This time, I make use of initializer powered by C itself. What is also highly recognizable is that I leverage a huge amount of macros which season the code to be more expressive, efficient as well as tasty:).

There are still something to add, when it comes to optimization. However, time is limited and I know I'm about to stop here.

## Project 2

Date: 14 Nov 2024

State: Finished all functionality related to mandatory and the second optional task (scoping).

I managed to lower the complexity and improve efficiency in the process of writing code. Before starting this project,
migrating to CMake from Makefile was an unavoidable hurdle, as I missed the convenient functionalities provided by
`CLion`. So, I started learning it the hard way.

After achieving that, I coincidentally noticed dynamic analysis tools nested in the depths of `CLion` settings. I went a
little astray for a while, but the benefits of mastering sanitizers gradually appeared when dealing with pointers. I am
now reassuringly peaceful even though lagging behind.

- Redirecting stdout to a file is helpful for reference.
- Using soft links to solve `include` errors.
- Adding `const` and `assert` statements, no matter how tiresome, prevents the program from derailing.
- A new way of constructing a linked list using double pointers:

```c
(*t)->array.elemType = NULL; // Initialize elemType to NULL
t = &(*t)->array.elemType;   // Update t to point to elemType
```

- Inside switch statements, initialization cannot be done directly without curly braces.
- A seemingly trivial error: I originally evaluated `Exp ASSIGNOP EXP` inside `resolveDef`. Alas, at this time variables haven't been inserted into `vMap`. So I squeeze through it by using a cumbersome function `checkInit`. Hope I will roll with it later.

## Project 1

Data: 7 Oct 2024

实现的功能
必选与选项一, 所要求的功能均已实现

### 总结

先从变成工具的使用说起，往常C语言的开发，我都在Clion中进行。但是这次，因为没有提供cmakelist，我只好在vscode
中进行。而恰好是这次，让我能够更深入地了解vscode。我学会了写task.json，了解到vscode还支持profile等等。我使用git进行历史记录的保留。这些都是非常宝贵的尝试。

前期，主要面临工具使用不熟练的问题。一方面，表现在上述的问题；另一方面，表现为不熟悉flex和bison的使用。对此二者编程几乎无工具可言，只能一个字符一个字符地敲上。这种绝无仅有的体验，让我感受到了纯粹痛苦之后的快乐。

让我印象较深的一个bug是，我在bison的文件中指定INT返回类型为int，
FLOAT返回类型为float，而其他则返回node。这么定义是方便flex返回值，比如检测到一个int，就返回对应的int值。后来，我发现这样不对，此举会让bison错误判断INT在bison文件中的返回值。具体而言，从flex到bison传递的是int类型的值，在bison中从INT到包含有INT的表达式的值依旧是int类型，然而本该是node类型，只有这样才能构建语法树。回望千钧一发之际，我不胜悲凉，原来debug是要靠直觉的……
