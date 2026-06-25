@tool
extends VBoxContainer
class_name LegendElement

@onready var Function : Label = $Function
@onready var FunctionColor : ColorRect = $Color

# Godot 4 : champs simples (les anciens setget G3 ne portaient aucune logique ;
# les port en property/setter G4 récurseraient sur l'assignation interne).
var function : String
var color : Color
var font_color : Color
var font : Font

func _ready():
	Function.set("theme_override_fonts/font", font)
	Function.set("theme_override_colors/font_color", font_color)
	Function.set_text(function)
	FunctionColor.set_frame_color(color)

func create_legend(text : String, _color : Color, _font : Font, _font_color : Color):
	self.function = text
	self.color = _color
	self.font_color = _font_color
	self.font = _font

func set_function( t : String ):
	function = t

func get_function() -> String:
	return function

func set_function_color( c : Color ):
	color = c

func get_function_color() -> Color:
	return color

# Renommé : on ne peut pas surcharger Object.get_class() en Godot 4.
func get_legend_class() -> String:
	return "Legend Element"

func _to_string() -> String:
	return "%s (%s, %s) " % [get_legend_class(), get_function(), get_function_color().to_html(true)]
