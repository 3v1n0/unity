<?xml version="1.0"?>
<api version="1.0">
	<namespace name="Unity">
		<object name="UnityPlace" parent="GObject" type-name="UnityPlace" get-type="unity_place_get_type">
			<property name="icon-name" type="char*" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="name" type="char*" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="tooltip" type="char*" readable="1" writable="1" construct="0" construct-only="0"/>
			<signal name="is-active" when="LAST">
				<return-type type="void"/>
				<parameters>
					<parameter name="place" type="UnityPlace*"/>
					<parameter name="is_active" type="gboolean"/>
				</parameters>
			</signal>
			<signal name="view-changed" when="LAST">
				<return-type type="void"/>
				<parameters>
					<parameter name="object" type="UnityPlace*"/>
					<parameter name="p0" type="char*"/>
					<parameter name="p1" type="GHashTable_gchararray+gchararray_*"/>
				</parameters>
			</signal>
		</object>
		<object name="UnityPlaceProxy" parent="GObject" type-name="UnityPlaceProxy" get-type="unity_place_proxy_get_type">
			<property name="icon-name" type="char*" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="name" type="char*" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="tooltip" type="char*" readable="1" writable="1" construct="0" construct-only="0"/>
			<signal name="ready" when="LAST">
				<return-type type="void"/>
				<parameters>
					<parameter name="proxy" type="UnityPlaceProxy*"/>
				</parameters>
			</signal>
		</object>
	</namespace>
</api>
