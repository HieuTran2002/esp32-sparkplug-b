# Still under development

# Installation
1. clone [nanopb](https://github.com/nanopb/nanopb)
- Generate C code and header file for Sparkplug_b
```
cd ./nanopb/generator/
python nanopb_generator.py --output-dir=./sparkplug_b sparkplug_b.proto

```
2. Copy generated files into main/ and start coding.
