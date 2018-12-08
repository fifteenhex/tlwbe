from pycparser import parse_file
from pycparser.c_ast import Typedef, TypeDecl, Struct, Decl, IdentifierType, PtrDecl
from enum import Enum


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


class FieldType(Enum):
    NORMAL = 1
    POINTER = 2
    STRUCT = 3


class Field:
    def __init__(self, field: Decl):
        field.show()

        self.field = field
        self.field_name = field.name
        if type(field.type) == TypeDecl and type(field.type.type) == IdentifierType:
            self.type = FieldType.NORMAL
            self.c_type = field.type.type.names[0]
        elif type(field.type) == PtrDecl:
            self.type = FieldType.POINTER
            self.c_type = field.type.type.type.names[0]
        elif type(field.type) == TypeDecl and type(field.type.type) == Struct:
            self.type = FieldType.STRUCT
        else:
            assert False, ("field type %s not handled" % type(field.type))


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
            fields.append(Field(field))

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
