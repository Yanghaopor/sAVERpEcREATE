# sAVERpEcREATE
基于Windwos的PE烧录程序，声明，使用了 https://github.com/mcmilk/7-Zip?tab=LGPL-2.1-1-ov-file 的解压功能

# SaverEpCreate应用以及开发讲解
因我们工作室对于Win系统定制进行研究，而不想使用

其它PE烧录程序，故此开发这个简单的程序。

## 原理
烧录本身是给复杂的事情，我看了许多开源项目，我并不想写一套

跨平台的烧录程序，于是我借助了Windows自带的硬盘管理程序 Desktop.exe 进行初始化操作

基于Desktop，我们成功的将移动存储设备改为FAS32或者NTFS格式，以及16kb对其度，

再使用Windows的分盘API给选定的盘进行MBR或者GPT分卷使其具备系统盘的功能。

最后使用 7zip 的API 进行简单的封装烧录，系统盘基本完成。

## 注意
这本身就是概念版，它不会检查系统完整性，遇到报错请告诉我谢谢，后面将会持续更新。

## 安全

### 自带硬盘检测，会检测是否为移动硬盘，如果不是将会停止程序，保护手残党 

### 兼容性
制作的PE系统不支持纯UEFI，因为U盘无法更改为GPT格式，所以不支持纯UEFI启动选项
注意：纯UEFI那是老爷机的玩意WinXP时代的东西，不会还有人用吧。

## 更新日志0.3
2025.4.21 13：00
https://github.com/Yanghaopor/sAVERpEcREATE/releases/tag/untagged-ff4033f390cc19136091
在我工作室的要求下我开发了对应的图形界面，这下更好操作了
这个图形界面基于我自己的Saver3RB应用框架
支持系统：windows10及以上，不支持windows10之下的系统
注意：如win7版本，请继续使用无图形化的版本

## 更新日志0.2
2025.4.27 00:52

https://github.com/Yanghaopor/sAVERpEcREATE/releases/tag/SaverPeCreate

此版本修复了对于移动盘FAT32的修复，使其不受FAT32对于系统镜像4GB大小

的限制。

注意：PE系统需要控制在1GB以内，UEFI系统分区只有1GB大小

注意：创建系统盘需要剩余2盘符，请不要把所有盘符用完，不然有数据丢失风险
