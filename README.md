# 2DDistanceFieldGenerator
根据2D图片生成距离场

# 其他想法

算的还是太慢

~~不过看在Spread Factor这个参数基本也就到256最大了，可以先程序生成一个以(0, 0)点为中心半径256的圆里的所有整数点，然后按距离排序存在一个txt文本里，我算了，也就2.5mb大的文件。然后直接顺着查找，找到了就跳出，应该能快不少。~~

加了多线程之后算的不算慢拉！
