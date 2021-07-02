# LP_Plugin_HumanFeatures

Plugin for [SmartFashioin](https://github.com/Lip-CPII/SmartFashion) that make use of PCA paramteric human modeling.
The plugin includes [OpenNURBS](https://www.rhino3d.com/opennurbs/) for mesh processing and great work for [QP solver](https://github.com/stack-of-tasks/eiquadprog)

## Usage
You may download the [binary](https://github.com/Lip-CPII/LP_Plugin_HumanFeatures/releases) then unzip and put it into the "_App/plugins_" directory or Build it yourself. 

** p.s. The binary is built from Ubuntu 20.04 64-bit ([v0.0.2](https://github.com/Lip-CPII/LP_Plugin_HumanFeatures/releases)). For Windows, you have to build it yourself by following instructions or download the prebuilt ([v0.0.3](https://github.com/Lip-CPII/LP_Plugin_HumanFeatures/releases)) and unzip them into the "_App/plugins_" directory or Build it yourself.


### Build
1. clone the repo. into the directory under "_SmartFashion_".
2. Edit the "_Smart_Fashion.pro_": In the *SUBDIRS*, add the cloned repo. name. E.g.
3. Right click on "_Smart_Fashion.pro_" and "_Run qmake_"
4. Run SmartFashion and "Human Features Making" should appears.
```
...
SUBDIRS += \
    App \
    Functional \
    LP_Plugin_HumanFeatures \
    ...
...
```

```bash

```

![image](https://user-images.githubusercontent.com/73818362/122339320-8127f400-cf73-11eb-86f7-56fd20558494.png)
