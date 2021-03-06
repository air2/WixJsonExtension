﻿using System.Reflection;
using System.Xml;
using System.Xml.Schema;
using Microsoft.Tools.WindowsInstallerXml;

namespace NerdyDuck.Wix.JsonExtension
{
	public sealed class JsonCompiler : CompilerExtension
	{
		public JsonCompiler()
		{
			Schema = LoadXmlSchemaHelper(Assembly.GetExecutingAssembly(), "NerdyDuck.Wix.JsonExtension.Xsd.json.xsd");
		}

		public override XmlSchema Schema
		{
			get;
		}

		/// <summary>
		/// Processes an element for the Compiler.
		/// </summary>
		/// <param name="sourceLineNumbers">Source line number for the parent element.</param>
		/// <param name="parentElement">Parent element of element to process.</param>
		/// <param name="element">Element to process.</param>
		/// <param name="contextValues">Extra information about the context in which this element is being parsed.</param>
		public override void ParseElement(SourceLineNumberCollection sourceLineNumbers, XmlElement parentElement, XmlElement element, params string[] contextValues)
		{
			switch (parentElement.LocalName)
			{
				case "Component":
					string componentId = contextValues[0];
					string directoryId = contextValues[1];

					switch (element.LocalName)
					{
						case "JsonFile":
							ParseJsonFileElement(element, componentId, directoryId);
							break;
						default:
							Core.UnexpectedElement(parentElement, element);
							break;
					}
					break;
				default:
					Core.UnexpectedElement(parentElement, element);
					break;
			}
		}

		/// <summary>
		/// Parses a RemoveFolderEx element.
		/// </summary>
		/// <param name="node">Element to parse.</param>
		/// <param name="componentId">Identifier of parent component.</param>
		/// <param name="parentDirectory">Identifier of parent component's directory.</param>
		private void ParseJsonFileElement(XmlNode node, string componentId, string parentDirectory)
		{
			SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
			string id = null;
			string file = null;
			string elementPath = null;
			string value = null;
			int on = CompilerCore.IntegerNotSet;
			int flags = 0;
			int action = CompilerCore.IntegerNotSet;
			int? sequence = 1;
			int selectionLanguage = CompilerCore.IntegerNotSet;
			int valueType = CompilerCore.IntegerNotSet;
			string verifyPath = null;

			if (node.Attributes != null)
			{
				foreach (XmlAttribute attribute in node.Attributes)
				{
					if (0 == attribute.NamespaceURI.Length || attribute.NamespaceURI == Schema.TargetNamespace)
					{
						switch (attribute.LocalName)
						{
							case "Id": 
								// Identifier for json file modification
								id = Core.GetAttributeIdentifierValue(sourceLineNumbers, attribute);
								break;
							case "File": 
								// Path of the .json file to modify.
								file = Core.GetAttributeValue(sourceLineNumbers, attribute);
								break;
							case "ElementPath": 
								// The path to the parent element of the element to be modified. The semantic can be either JSON Path or JSON Pointer language, as specified in the
								// SelectionLanguage attribute. Note that this is a formatted field and therefore, square brackets in the path must be escaped. In addition, JSON Path
								// and Pointer allow backslashes to be used to escape characters, so if you intend to include literal backslashes, you must escape them as well by doubling
								// them in this attribute. The string is formatted by MSI first, and the result is consumed as the JSON Path or Pointer.
								elementPath = Core.GetAttributeValue(sourceLineNumbers, attribute);
								break;
							case "Value": 
								// The value to set. May be one of the simple JSON types, or a JSON-formatted object. See the
								// <html:a href="http://msdn.microsoft.com/library/aa368609(VS.85).aspx" target="_blank">Formatted topic</html:a> for information how to escape square brackets in the value.
								value = Core.GetAttributeValue(sourceLineNumbers, attribute);
								break;
							case "Action": 
								// The type of modification to be made to the JSON file when the component is installed or uninstalled.
								action = ValidateAction(node, sourceLineNumbers, attribute, ref flags);
								break;
							case "On": 
								// Defines when the specified changes to the JSON file are to be done.
								on = ValidateOn(node, sourceLineNumbers, attribute, ref flags);
								break;
							case "PreserveModifiedDate": 
								// Specifies whether or not the modification should preserve the modified date of the file.  Preserving the modified date will allow
								// the file to be patched if no other modifications have been made.
								flags = ValidatePreserveModifiedDate(node, sourceLineNumbers, attribute, flags);
								break;
							case "Sequence": 
								// Specifies the order in which the modification is to be attempted on the JSON file.  It is important to ensure that new elements are created before you attempt to modify them.
								sequence = ToNullableInt(Core.GetAttributeValue(sourceLineNumbers, attribute));
								break;
							case "SelectionLanguage": 
								// Specify whether the JSON object should use JSON Path (default) or JSON Pointer as the query language for ElementPath.
								selectionLanguage = ValidateSelectionLanguage(node, sourceLineNumbers, attribute, ref flags);
								break;
							case "ValueType": 
								// Specify whether the JSON object should use JSON Path (default) or JSON Pointer as the query language for ElementPath.
								valueType = ValidateValueType(node, sourceLineNumbers, attribute, ref flags);
								break;
							case "VerifyPath": 
								// The path to the element being modified. This is required for 'delete' actions. For 'set' actions, VerifyPath is used to decide if the element already exists.
								// The semantic can be either JSON Path or JSON Pointer language, as specified in the SelectionLanguage attribute. Note that this is a formatted field and therefore,
								// square brackets in the path must be escaped. In addition, JSON Path and Pointer allow backslashes to be used to escape characters, so if you intend to include literal
								// backslashes, you must escape them as well by doubling them in this attribute. The string is formatted by MSI first, and the result is consumed as the JSON Path or Pointer.
								verifyPath = Core.GetAttributeValue(sourceLineNumbers, attribute);
								break;
							default:
								Core.UnexpectedAttribute(sourceLineNumbers, attribute);
								break;
						}
					}
					else
					{
						Core.UnsupportedExtensionAttribute(sourceLineNumbers, attribute);
					}
				}
			}

			if (CompilerCore.IntegerNotSet == action)
			{
				// default is set value
				flags |= 2;
			}

			foreach (XmlNode child in node.ChildNodes)
			{
				if (XmlNodeType.Element != child.NodeType)
				{
					continue;
				}

				if (child.NamespaceURI == Schema.TargetNamespace)
				{
					Core.UnexpectedElement(node, child);
				}
				else
				{
					Core.UnsupportedExtensionElement(node, child);
				}
			}

			if (Core.EncounteredError)
			{
				return;
			}

			Row row = Core.CreateRow(sourceLineNumbers, "JsonFile");
			int rowIdx = 0;
			row[rowIdx++] = id;
			row[rowIdx++] = file;
			row[rowIdx++] = elementPath;
			row[rowIdx++] = verifyPath;
			row[rowIdx++] = value;
			row[rowIdx++] = flags;
			row[rowIdx++] = componentId;
			row[rowIdx] = sequence;

			Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "JsonFile");
		}

		private int ValidatePreserveModifiedDate(XmlNode node, SourceLineNumberCollection sourceLineNumbers,
			XmlAttribute attribute, int flags)
		{
			string preserveModifiedDateValue = Core.GetAttributeValue(sourceLineNumbers, attribute);
			if (preserveModifiedDateValue.Length == 0)
			{
			}
			else
			{
				switch (preserveModifiedDateValue)
				{
					case "yes":
						flags |= 16;
						break;
					case "no":
						break;
					default:
						Core.OnMessage(WixErrors.IllegalAttributeValue(sourceLineNumbers, node.Name,
							"PreserveModifiedDate", preserveModifiedDateValue, "yes", "no"));
						break;
				}
			}

			return flags;
		}

		private int ValidateSelectionLanguage(XmlNode node, SourceLineNumberCollection sourceLineNumbers,
			XmlAttribute attribute, ref int flags)
		{
			int selectionLanguage;
			string selectionLanguageValue = Core.GetAttributeValue(sourceLineNumbers, attribute);
			if (selectionLanguageValue.Length == 0)
			{
				selectionLanguage = CompilerCore.IllegalInteger;
			}
			else
			{
				switch (selectionLanguageValue)
				{
					case "JSONPath":
						selectionLanguage = 1;
						break;
					case "JSONPointer":
						flags |= 32;
						selectionLanguage = 2;
						break;
					default:
						Core.OnMessage(WixErrors.IllegalAttributeValue(sourceLineNumbers, node.Name,
							"SelectionLanguage", selectionLanguageValue, "JSONPath", "JSONPointer"));
						selectionLanguage = CompilerCore.IllegalInteger;
						break;
				}
			}

			return selectionLanguage;
		}

		private int ValidateValueType(XmlNode node, SourceLineNumberCollection sourceLineNumbers,
			XmlAttribute attribute, ref int flags)
		{
			int valueType;
			string valueTypeValue = Core.GetAttributeValue(sourceLineNumbers, attribute);
			if (valueTypeValue.Length == 0)
			{
				valueType = 1;
			}
			else
			{
				switch (valueTypeValue)
				{
					case "string":
						valueType = 1;
						break;
					case "bool":
						flags |= 64;
						valueType = 2;
						break;
					case "number":
						flags |= 128;
						valueType = 3;
						break;
					case "object":
						flags |= 256;
						valueType = 4;
						break;
					case "null":
						flags |= 512;
						valueType = 5;
						break;
					default:
						Core.OnMessage(WixErrors.IllegalAttributeValue(sourceLineNumbers, node.Name,
							"ValueType", valueTypeValue, "string", "bool", "number","object", "null"));
						valueType = CompilerCore.IllegalInteger;
						break;
				}
			}

			return valueType;
		}

		private int ValidateOn(XmlNode node, SourceLineNumberCollection sourceLineNumbers, XmlAttribute attribute,
			ref int flags)
		{
			int on;
			string onValue = Core.GetAttributeValue(sourceLineNumbers, attribute);
			if (onValue.Length == 0)
			{
				on = CompilerCore.IllegalInteger;
			}
			else
			{
				switch (onValue)
				{
					case "install":
						on = 1;
						break;
					case "uninstall":
						flags |= 8;
						on = 2;
						break;
					case "both":
						on = 3;
						break;
					default:
						Core.OnMessage(WixErrors.IllegalAttributeValue(sourceLineNumbers, node.Name,
							"On", onValue, "install", "uninstall", "both"));
						on = CompilerCore.IllegalInteger;
						break;
				}
			}

			return on;
		}

		private int ValidateAction(XmlNode node, SourceLineNumberCollection sourceLineNumbers, XmlAttribute attribute,
			ref int flags)
		{
			const string ActionDeleteValue = "deleteValue";
			const string ActionSetValue = "setValue";
			const string ActionAddArrayValue = "addArrayValue";

			int action;
			string actionValue = Core.GetAttributeValue(sourceLineNumbers, attribute);
			if (actionValue.Length == 0)
			{
				action = CompilerCore.IllegalInteger;
			}
			else
			{
				switch (actionValue)
				{
					case ActionDeleteValue:
						flags |= 1;
						action = 1;
						break;
					case ActionSetValue:
						flags |= 2;
						action = 2;
						break;
					case ActionAddArrayValue:
						flags |= 4;
						action = 3;
						break;
					default:
						Core.OnMessage(WixErrors.IllegalAttributeValue(sourceLineNumbers, node.Name,
							"Action", actionValue, ActionDeleteValue, ActionSetValue, ActionAddArrayValue));
						action = CompilerCore.IllegalInteger;
						break;
				}
			}

			return action;
		}

		public int? ToNullableInt(string s)
		{
			if (int.TryParse(s, out int i))
			{
				return i;
			}
			return null;
		}
	}

}
