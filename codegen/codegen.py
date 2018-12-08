from pycparser import parse_file
from pycparser.c_ast import Typedef, Struct, Decl


def annotation_type_from_field_name(field_name: str):
    return field_name[2:].split('_')[1]


def annotation_parameters_from_field_name(field_name: str):
    return field_name[2:].split('_')[2:]


class AnnotatedStruct:
    def __init__(self, struct_name: str, annotation_type: str, parameters: list):
        self.struct_name = struct_name
        self.annotation_type = annotation_type
        self.parameters = parameters


class FieldAnnotation:
    def __init__(self, field_name):
        self.annotation_type = annotation_type_from_field_name(field_name)
        all_parameters = annotation_parameters_from_field_name(field_name)
        self.field_name = all_parameters[0]
        self.parameters = all_parameters[1:]


class CodeBlock:
    def write(self, outputfile):
        outputfile.write('// empty code block\n\n')


def __fulltag(tag: str):
    return '__%s' % tag


def parsefile(tag: str, input):
    barrier = '-D__%s' % tag.upper()
    ast = parse_file(input, use_cpp=True,
                     cpp_args=[barrier, '-I/usr/share/python3-pycparser/fake_libc_include'])
    return ast


def find_annotated_structs(tag: str, annotation_types: list, ast):
    annotated_structs = []

    for child in ast.ext:
        if type(child) is Typedef:
            if child.name.startswith(__fulltag(tag)):
                name_parts = child.name[2:].split('_')

                assert len(name_parts) >= 2
                annotation_type = name_parts[1]
                assert annotation_type in annotation_types

                struct_name = child.type.type.name

                parameters = name_parts[2:]
                annotated_structs.append(AnnotatedStruct(struct_name, annotation_type, parameters))
                print("%s : %s -> %s" % (struct_name, annotation_type, str(parameters)))

    return annotated_structs


def find_structs(ast, callback, data):
    outputs = []
    for child in ast.ext:
        if type(child) is Decl:
            if type(child.type) is Struct:
                callback(child.type, data, outputs)

    return outputs


def walk_struct(tag: str, struct: Struct, annotation_types=[]):
    annotations = []
    fields = []

    # bucket the fields and the annotations
    for field in struct:
        if field.name.startswith(__fulltag(tag)):
            print("found annotation %s" % field.name)
            assert annotation_type_from_field_name(field.name) in annotation_types
            annotations.append(FieldAnnotation(field.name))
        else:
            print("found field %s" % field.name)
            fields.append(field)

    return fields, annotations


def struct_by_name(ast, name: (str)):
    print('looking for struct %s' % name)
    for child in ast:
        if type(child) is Decl:
            if type(child.type) is Struct:
                if child.type.name == name:
                    print('found struct %s' % name)
                    return child.type
    return None
