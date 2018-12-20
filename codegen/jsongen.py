#!/usr/bin/env python3

import argparse
import codegen
from pycparser.c_ast import Struct
from enum import Enum

parser = argparse.ArgumentParser(description='json code gen')
parser.add_argument('--input', type=str, required=True)
parser.add_argument('--output', type=str, required=True)

TAG = 'jsongen'

annotation_types = {
    'flags', 'default'
}


def flatten_path(path, field_name):
    field_path = path.copy()
    field_path.append(field_name)
    return ".".join(field_path)


class JsonFieldType(Enum):
    STRING = 0
    INT = 1
    DOUBLE = 2
    BOOLEAN = 3
    OBJECT = 4
    BASE64BLOB = 5


class JsonField:
    __slots__ = ['name', 'type', 'children', 'c_field', 'annotations']

    def __init__(self, name: str, type: JsonFieldType, c_field=None, annotations=None):
        self.name = name
        self.type = type
        self.c_field = c_field
        self.annotations = annotations
        self.children = []


class JsonCodeBlock(codegen.CodeBlock):
    __slots__ = ['struct_name', 'fields_and_annotations', 'root']

    __type_mapping = {
        'guint64': JsonFieldType.INT,
        'guint32': JsonFieldType.INT,
        'guint16': JsonFieldType.INT,
        'guint8': JsonFieldType.INT,
        'gint64': JsonFieldType.INT,
        'gint32': JsonFieldType.INT,
        'gint16': JsonFieldType.INT,
        'gint8': JsonFieldType.INT,
        'gsize': JsonFieldType.INT,
        'gboolean': JsonFieldType.BOOLEAN,
        'gdouble': JsonFieldType.DOUBLE
    }

    __pointer_type_mapping = {
        'gchar': JsonFieldType.STRING,
        'guint8': JsonFieldType.BASE64BLOB
    }

    def __dowalk(self, root: JsonField, fields_and_annotations):
        for field in fields_and_annotations[0]:
            print(field.field_name)
            if field.type == codegen.FieldType.STRUCT:
                new_root = JsonField(field.field_name, JsonFieldType.OBJECT, field.field_name)
                self.__dowalk(new_root, field.fields_and_annotations)
                root.children.append(new_root)
                continue
            elif field.type == codegen.FieldType.POINTER:
                json_type = self.__pointer_type_mapping.get(field.c_type)
            else:
                json_type = self.__type_mapping.get(field.c_type)
            assert json_type is not None, ('no json field mapping for %s' % field.c_type)

            field_annotations = []
            for annotation in fields_and_annotations[1]:
                if annotation.field_name == field.field_name:
                    field_annotations.append(annotation)

            root.children.append(JsonField(field.field_name, json_type, field, field_annotations))

    def __init__(self, struct_name: str, fields_and_annotations):
        super(JsonCodeBlock, self).__init__()
        self.struct_name = struct_name
        self.fields_and_annotations = fields_and_annotations
        self.root = JsonField(None, JsonFieldType.OBJECT)
        self.__dowalk(self.root, fields_and_annotations)


class JsonParser(JsonCodeBlock):
    def __init__(self, struct_name: str, fields_and_annotations):
        super().__init__(struct_name, fields_and_annotations)

    def __get_int(self, field: codegen.Field, path, outputfile):
        self.add_statement('%s->%s = json_object_get_int_member(root, "%s")' % (
            self.struct_name, flatten_path(path, field.field_name), field.field_name), outputfile)

    def __get_boolean(self, field: codegen.Field, path, outputfile):
        self.add_statement('%s->%s = json_object_get_boolean_member(root, "%s")' % (
            self.struct_name, flatten_path(path, field.field_name), field.field_name), outputfile)

    def __get_double(self, field: codegen.Field, path, outputfile):
        self.add_statement('%s->%s = json_object_get_double_member(root, "%s");\n' % (
            self.struct_name, flatten_path(path, field.field_name), field.field_name), outputfile)

    def __get_string(self, field: codegen.Field, path, outputfile):
        self.add_statement('%s->%s = json_object_get_string_member(root, "%s")' % (
            self.struct_name, flatten_path(path, field.field_name), field.field_name), outputfile)

    def __get_base64blob(self, field: codegen.Field, path, outputfile):
        self.start_scope(outputfile)
        self.add_statement('gchar* payloadb64 = json_object_get_string_member(root, "%s")' % field.field_name,
                           outputfile)
        self.add_statement('%s->%s = g_base64_decode(payloadb64, &%s->%slen)' % (
            self.struct_name, flatten_path(path, field.field_name), self.struct_name,
            flatten_path(path, field.field_name)),
                           outputfile)
        self.end_scope(outputfile)

    def __write(self, field: JsonField, path=[]):

        if field is not self.root:
            self.start_condition('json_object_has_member(root, "%s")' % field.name, outputfile)

        if field.type == JsonFieldType.OBJECT:
            if field.c_field is not None:
                path.append(field.c_field)
            for c in field.children:
                self.__write(c, path.copy())
        elif field.type == JsonFieldType.INT:
            self.__get_int(field.c_field, path, outputfile)
        elif field.type == JsonFieldType.BOOLEAN:
            self.__get_boolean(field.c_field, path, outputfile)
        elif field.type == JsonFieldType.DOUBLE:
            self.__get_double(field.c_field, path, outputfile)
        elif field.type == JsonFieldType.STRING:
            self.__get_string(field.c_field, path, outputfile)
        elif field.type == JsonFieldType.BASE64BLOB:
            self.__get_base64blob(field.c_field, path, outputfile)
        else:
            assert False, ('couldn\'t write json type %s' % field.type)

        if field is not self.root:
            self.end_condition(outputfile)

    def write(self, outputfile):
        self.start_scope(outputfile,
                         prefix='static gboolean __attribute__((unused)) __%s_%s_from_json(struct %s* %s, JsonObject* root)' % (
                             TAG, self.struct_name, self.struct_name, self.struct_name))
        self.__write(self.root)
        self.add_statement('return TRUE', outputfile)
        self.add_label('err', outputfile)
        self.add_statement('return FALSE', outputfile)
        outputfile.write('}\n\n')


class JsonBuilder(JsonCodeBlock):

    def __add_int(self, field: codegen.Field, path, outputfile):
        outputfile.write('\tjson_builder_add_int_value(jsonbuilder, %s->%s);\n' % (
            self.struct_name, flatten_path(path, field.field_name)))

    def __add_double(self, field: codegen.Field, path, outputfile):
        outputfile.write('\tjson_builder_add_double_value(jsonbuilder, %s->%s);\n' % (
            self.struct_name, flatten_path(path, field.field_name)))

    def __add_string(self, field: codegen.Field, path, outputfile):
        outputfile.write('\tjson_builder_add_string_value(jsonbuilder, %s->%s);\n' % (
            self.struct_name, flatten_path(path, field.field_name)))

    def __add_base64blob(self, field: codegen.Field, path, outputfile):
        self.start_scope(outputfile)
        outputfile.write('\t\tgchar * payloadb64 = g_base64_encode(%s->%s, %s->%slen);\n' % (
            self.struct_name, field.field_name, self.struct_name, flatten_path(path, field.field_name)))
        outputfile.write('\t\tjson_builder_add_string_value(jsonbuilder, payloadb64);\n')
        outputfile.write('\t\tg_free(payloadb64);\n')
        self.end_scope(outputfile)

    def __init__(self, struct_name: str, fields_and_annotations):
        super().__init__(struct_name, fields_and_annotations)

    @staticmethod
    def __set_field_name(name: str, outputfile):
        outputfile.write('\tjson_builder_set_member_name(jsonbuilder, "%s");\n' % name)

    @staticmethod
    def __begin_object(outputfile):
        outputfile.write('\tjson_builder_begin_object(jsonbuilder);\n')

    @staticmethod
    def __end_object(outputfile):
        outputfile.write('\tjson_builder_end_object(jsonbuilder);\n')

    def __write(self, field: JsonField, path=[]):
        if field.name is not None:
            self.__set_field_name(field.name, outputfile)
        if field.type == JsonFieldType.OBJECT:
            if field.c_field is not None:
                path.append(field.c_field)
            self.__begin_object(outputfile)
            for c in field.children:
                self.__write(c, path.copy())
            self.__end_object(outputfile)
        elif field.type == JsonFieldType.INT:
            self.__add_int(field.c_field, path, outputfile)
        elif field.type == JsonFieldType.DOUBLE:
            self.__add_double(field.c_field, path, outputfile)
        elif field.type == JsonFieldType.STRING:
            self.__add_string(field.c_field, path, outputfile)
        elif field.type == JsonFieldType.BASE64BLOB:
            self.__add_base64blob(field.c_field, path, outputfile)
        else:
            assert False, ('couldn\'t write json type %s' % field.type)

    def write(self, outputfile):
        outputfile.write(
            'static void __attribute__((unused)) __%s_%s_to_json(const struct %s* %s, JsonBuilder* jsonbuilder){\n' % (
                TAG, self.struct_name, self.struct_name, self.struct_name))
        self.__write(self.root)
        outputfile.write('}\n\n')


def __generate_parser(struct_name: str, fields_and_annotations):
    return JsonParser(struct_name, fields_and_annotations)


def __generate_builder(struct_name: str, fields_and_annotations):
    return JsonBuilder(struct_name, fields_and_annotations)


flag_to_generator = {
    'parser': __generate_parser,
    'builder': __generate_builder
}


def __struct_callback(ast, struct: Struct, flags: dict, outputs: list):
    f = flags.get(struct.name)
    if f is not None:
        print('found flags for %s' % struct.name)
        fields_and_annotations = codegen.walk_struct(ast, TAG, struct)
        for ff in f:
            outputs.append(flag_to_generator[ff](struct.name, fields_and_annotations))


if __name__ == '__main__':
    args = parser.parse_args()
    print("%s processing %s -> %s" % (TAG, args.input, args.output))

    ast = codegen.parsefile(TAG, args.input)
    annotated_structs = codegen.find_annotated_structs(TAG, ['parser', 'builder'], ast)

    flags = {}

    for annotated_struct in annotated_structs:
        f = flags.get(annotated_struct.struct_name)
        if f is None:
            f = []
            flags[annotated_struct.struct_name] = f
        f.append(annotated_struct.annotation_type)

    outputs = codegen.find_structs(ast, __struct_callback, flags)

    outputfile = open(args.output, 'w+')

    codegen.HeaderBlock(TAG, args.input).write(outputfile)
    for cb in outputs:
        cb.write(outputfile)
