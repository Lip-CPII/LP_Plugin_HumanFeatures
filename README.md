# LP_Plugin_HumanFeatures

Plugin for [SmartFashioin](https://github.com/Lip-CPII/SmartFashion) that make use of PCA paramteric human modeling.
The plugin includes [OpenNURBS](https://www.rhino3d.com/opennurbs/) for mesh processing and great work for [QP solver](https://github.com/stack-of-tasks/eiquadprog)

## Usage
You may download the [binary](https://github.com/Lip-CPII/LP_Plugin_HumanFeatures/releases) then unzip and put it into the "_App/plugins_" directory or Build it yourself. 

** p.s. The binary is built from Ubuntu 20.04 64-bit. For Windows, you have to build it yourself by following instructions.


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

![image](https://user-images.githubusercontent.com/73818362/121985900-34ec8080-cdc8-11eb-84bd-e02be575472a.png)
